#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BACKLOG 10
#define BUFSIZE 8192

static void
usage ()
{
  printf ("Usage:\n");
  printf ("server DIRECTORY PORT\n");
  printf ("DIRECTORY: directory to the image files.\n");
  printf ("PORT: port to accept or send file.\n");
}

int
main (int agrc, const char **argv)
{
  if (agrc != 3)
    {
      usage ();
      return 1;
    }

  // Change the current directory
  int status = chdir (argv[1]);
  if (status == -1)
    goto error;

  // Open the current directory
  DIR *dir = opendir (".");
  if (dir == NULL)
    goto error;

  struct addrinfo hints;
  struct addrinfo *res;
  memset (&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; /* don't care IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* TCP stream sockets */
  hints.ai_flags = AI_PASSIVE; /* fill in my IP for me */
  
  status = getaddrinfo (NULL, argv[2], &hints, &res);
  if (status != 0)
    goto error;

  // make a socket:
  int sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);

  if (sockfd == -1)
    goto error;

  // bind it to the port we passed in to getaddrinfo():
  status = bind (sockfd, res->ai_addr, res->ai_addrlen);
  if (status != 0)
    goto error;

  status = listen (sockfd, BACKLOG);
  if (status != 0)
    goto error;

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof their_addr;
  int new_fd = accept (sockfd, (struct sockaddr *) &their_addr, &addr_size);

  recv (new_fd, &status, sizeof status, MSG_EOR);
  printf ("Connection established! %d\n", status);

  // Calculate the time.
  struct timespec start, end;
  clock_gettime (CLOCK_MONOTONIC_RAW, &start);

  struct dirent *imgFile = readdir (dir);
  ssize_t jobSize = 0;
  while (imgFile != NULL)
    {
      int filefd = open (dir->d_name, O_RDONLY);

      // Transfer the file
      ssize_t bytes = sendfile (new_fd, filefd, NULL, BUFSIZE);
      while (bytes != 0)
        {
          jobSize += bytes;
          bytes = sendfile (new_fd, filefd, NULL, BUFSIZE);
        }

      close (filefd);
      imgFile = readdir (dir);
    }

  recv (new_fd, &status, sizeof status, MSG_EOR);
  printf ("Processing terminated! %d\n", status);

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  time_t delta_s = end.tv_sec - start.tv_sec;
  long delta_us = end.tv_nsec - start.tv_nsec;
  long double delta = (long double) delta_s + (long double) delta_us / (long double) 1e9;
  printf ("Time cost: %.2Lf seconds\n", delta);

  close (new_fd);
  close (sockfd);
  freeaddrinfo (res);
  closedir (dir);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
