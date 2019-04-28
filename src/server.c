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
#define MAX_LEN 1024

static void
usage ()
{
  printf ("Usage:\n");
  printf ("server FILENAME PORT\n");
  printf ("FILENAME: file to send or receive.\n");
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

  int status;
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

  status = listen(sockfd, BACKLOG);
  if (status != 0)
    goto error;

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof their_addr;
  int new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);

  int i;
  recv (new_fd, &i, sizeof i, MSG_EOR);
  printf ("Connection established! %d\n", i);

  // Calculate the time.
  struct timespec start, end;
  clock_gettime (CLOCK_MONOTONIC_RAW, &start);

  char buf[MAX_LEN];
  // mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int filefd = open (argv[1], O_RDONLY);
  if (filefd == -1)
    goto error;

  ssize_t bytes = read (filefd, buf, MAX_LEN);
  while (bytes != 0)
    {
      send (new_fd, buf, bytes, MSG_EOR);
      bytes = read (filefd, buf, MAX_LEN);
    }

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  time_t delta_s = end.tv_sec - start.tv_sec;

  struct stat file_stat;
  stat (argv[1], &file_stat);
  off_t size = file_stat.st_size;
  printf ("File size: %ld bytes\n", size);
  printf ("Time cost: %ld seconds\n", delta_s);
  printf ("Speed: %.2Lf Mbps\n", (long double) size / (long double) delta_s * 8.0 / 1000000.0);

  close (filefd);
  close (new_fd);
  close (sockfd);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
