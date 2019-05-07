#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BACKLOG 10

static void
usage ()
{
  printf ("Usage:\n");
  printf ("server DIRECTORY PORT [IP]\n");
  printf ("DIRECTORY: directory to the image files.\n");
  printf ("PORT: port to accept or send file.\n");
  printf ("IP: the address of client, use passive mode if not set.\n");
}

int
main (int agrc, const char **argv)
{
  if (agrc != 3 && agrc != 4)
    {
      usage ();
      return 1;
    }
  const char *directory = argv[1];
  const char *port = argv[2];
  const char *address = NULL;
  if (agrc == 4)
    address = argv[3];

  // Change the current directory
  int status = chdir (directory);
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
  if (address == NULL)
    hints.ai_flags = AI_PASSIVE; /* fill in my IP for me */
  
  status = getaddrinfo (address, port, &hints, &res);
  if (status != 0)
    goto error;

  // make a socket:
  int sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);

  if (sockfd == -1)
    goto error;

  int new_fd;
  if (address == NULL)
    {
      // bind it to the port we passed in to getaddrinfo():
      status = bind (sockfd, res->ai_addr, res->ai_addrlen);
      if (status != 0)
        goto error;

      status = listen (sockfd, BACKLOG);
      if (status != 0)
        goto error;

      struct sockaddr_storage their_addr;
      socklen_t addr_size = sizeof their_addr;
      new_fd = accept (sockfd, (struct sockaddr *) &their_addr, &addr_size);
    }
  else
    {
      // connect!
      status = connect (sockfd, res->ai_addr, res->ai_addrlen);
      if (status != 0)
        goto error;
      new_fd = sockfd;
    }

  recv (new_fd, &status, sizeof status, MSG_EOR);
  printf ("Connection established! %d\n", status);

  // Calculate the time.
  struct timespec start, end;
  clock_gettime (CLOCK_MONOTONIC_RAW, &start);

  struct dirent *imgFile = readdir (dir);
  ssize_t jobSize = 0;
  while (imgFile != NULL)
    {
      if (imgFile->d_name[0] != '.')
        {
          int filefd = open (imgFile->d_name, O_RDONLY);
          struct stat statbuf;
          fstat (filefd, &statbuf);
          jobSize += statbuf.st_size;

          // Tell the client the size
          send (new_fd, &(statbuf.st_size), sizeof (statbuf.st_size), MSG_EOR);

          // Transfer the file
          sendfile (new_fd, filefd, NULL, statbuf.st_size);

          close (filefd);
        }
      imgFile = readdir (dir);
    }

  // Tell the client to terminate
  off_t terminate = 0;
  send (new_fd, &terminate, sizeof terminate, MSG_EOR);

  recv (new_fd, &status, sizeof status, MSG_EOR);
  printf ("Client finished!\n");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  time_t delta_s = end.tv_sec - start.tv_sec;
  long delta_us = end.tv_nsec - start.tv_nsec;
  long double delta = (long double) delta_s + (long double) delta_us / (long double) 1e9;
  printf ("%ld bytes transferred\n", jobSize);
  printf ("Time cost: %.2Lf seconds\n", delta);
  printf ("Bandwidth: %.2Lf Mbps\n", (long double) jobSize / delta * 8.0 / 1000000.0);

  close (new_fd);
  if (address == NULL)
    close (sockfd);
  freeaddrinfo (res);
  closedir (dir);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
