#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <turbojpeg.h>
#include <unistd.h>

// Maximum file size (1 MiB)
#define MAXFILE 1048576

// Maximum raw file size (64 MiB)
#define MAXRAW 67108864

#define BUFSIZE 8192

#define BACKLOG 10

const char *subsampName[TJ_NUMSAMP] = {
  "4:4:4", "4:2:2", "4:2:0", "Grayscale", "4:4:0", "4:1:1"
};

const char *colorspaceName[TJ_NUMCS] = {
  "RGB", "YCbCr", "GRAY", "CMYK", "YCCK"
};

static void
usage ()
{
  printf ("Usage:\n");
  printf ("client DIRECTORY PORT [IP]\n");
  printf ("DIRECTORY: directory to the image files.\n");
  printf ("PORT: port to accept or send file.\n");
  printf ("IP: the address of client, use passive mode if not set.\n");
}

// Retrieve SIZE data from SOCKFD
static int
retrieve (int sockfd, void *buffer, off_t size)
{
  ssize_t offset = 0, bytes;
  while (offset < size)
    {
      bytes = recv (sockfd, buffer + offset, size - offset, MSG_EOR);
      if (bytes == -1)
        return -1;
      offset += bytes;
    }
  return 0;
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

  tjhandle handle = tjInitDecompress ();

  int status;
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

  // Notify the server
  status = 10;
  send (new_fd, &status, sizeof status, MSG_EOR);

  unsigned char *buf = malloc (MAXFILE);
  unsigned char *raw = malloc (MAXRAW);
  off_t fileSize;
  ssize_t jobSize = 0;
  char fullName[300];
  int fileNum = 1;
  status = retrieve (new_fd, &fileSize, sizeof fileSize);
  while (fileSize != 0)
    {
      printf ("File size: %ld\n", fileSize);
      status = retrieve (new_fd, buf, fileSize);
      jobSize += fileSize;
      int width, height, jpegSubsamp, jpegColorspace;

      // Get the header info
      tjDecompressHeader3 (handle, buf, fileSize,
        &width, &height, &jpegSubsamp, &jpegColorspace);
      printf ("Input Image:  %d x %d pixels, %s subsampling, %s colorspace\n",
        width, height, subsampName[jpegSubsamp], colorspaceName[jpegColorspace]);

      // Decompress the jpeg into RGB
      tjDecompress2 (handle, buf, fileSize, raw, width, 0, height, TJPF_RGB, 0);

      // Write the RGB file
      snprintf (fullName, 300, "%s/%d.jpg", directory, fileNum);
      mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      int outfd = open (fullName, O_WRONLY | O_CREAT | O_TRUNC, mode);
      write (outfd, raw, width * height * tjPixelSize[TJPF_RGB]);
      close (outfd);
      fileNum++;

      // Get the next file size.
      status = retrieve (new_fd, &fileSize, sizeof fileSize);
    }
  printf ("%ld bytes received\n", jobSize);

  close (new_fd);
  if (address == NULL)
    close (sockfd);
  freeaddrinfo (res);
  tjDestroy (handle);
  free (buf);
  free (raw);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
