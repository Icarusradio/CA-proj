#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LEN 1024

static void
usage ()
{
  printf ("Usage:\n");
  printf ("server FILENAME PORT SERVER_IP\n");
  printf ("FILENAME: file to send or receive.\n");
  printf ("PORT: port to accept or send file.\n");
  printf ("SERVER_IP: the server's IP.\n");
}

int
main (int agrc, const char **argv)
{
  if (agrc != 4)
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
  
  status = getaddrinfo (argv[3], argv[2], &hints, &res);
  if (status != 0)
    goto error;

  // make a socket:
  int sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);

  if (sockfd == -1)
    goto error;

  // connect!
  status = connect(sockfd, res->ai_addr, res->ai_addrlen);
  if (status != 0)
    goto error;

  int i = 10;
  send (sockfd, &i, sizeof i, MSG_EOR);

  char buf[MAX_LEN];
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int filefd = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (filefd == -1)
    goto error;

  ssize_t bytes = recv (sockfd, buf, MAX_LEN, MSG_EOR);
  while (bytes != 0)
    {
      write (filefd, buf, bytes);
      bytes = recv (sockfd, buf, MAX_LEN, MSG_EOR);
    }

  close (filefd);
  close (sockfd);

  return 0;

 error:
  fprintf(stderr, "Error!\n");
  return 1;
}
