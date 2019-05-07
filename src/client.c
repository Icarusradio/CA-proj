#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Maximum file size (1 MiB)
#define MAXFILE 1048576

#define BUFSIZE 8192

static void
usage ()
{
  printf ("Usage:\n");
  printf ("server SERVER_IP PORT\n");
  printf ("SERVER_IP: the server's IP.\n");
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
  
  status = getaddrinfo (argv[1], argv[2], &hints, &res);
  if (status != 0)
    goto error;

  // make a socket:
  int sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);

  if (sockfd == -1)
    goto error;

  // connect!
  status = connect (sockfd, res->ai_addr, res->ai_addrlen);
  if (status != 0)
    goto error;

  // Notify the server
  status = 10;
  send (sockfd, &status, sizeof status, MSG_EOR);

  char buf[MAXFILE];
  ssize_t bytes = recv (sockfd, buf, BUFSIZE, MSG_EOR);
  off_t offset = bytes;
  while (bytes != 0)
    {
      bytes = recv (sockfd, buf + offset, BUFSIZE, MSG_EOR);
      if (bytes != BUFSIZE)
        {
          offset = 0;

          // Compress the image
        }
      else
        offset += bytes;
    }

  // Tell the server that we are done
  status = 10;
  send (sockfd, &status, sizeof status, MSG_EOR);

  close (sockfd);
  freeaddrinfo (res);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
