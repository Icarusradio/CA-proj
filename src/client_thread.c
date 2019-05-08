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
#include "atomic_queue.h"

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

struct job_elem
  {
    struct list_elem elem;
    unsigned char *buf;
    off_t fileSize;
    char fullName[50];
  };

static struct atomic_queue queue;

static bool transmit = true;

static void
usage ()
{
  printf ("Usage:\n");
  printf ("client DIRECTORY THREADS PORT [IP]\n");
  printf ("DIRECTORY: directory to the image files.\n");
  printf ("THREADS: number of threads.\n");
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

static void *
thread_decompress (void *aux UNUSED)
{
  tjhandle handle = tjInitDecompress ();
  int width, height, jpegSubsamp, jpegColorspace;
  unsigned char *raw = malloc (MAXRAW);
  struct list_elem *e = atomic_queue_pop (&queue);
  while (transmit || e != NULL)
    {
      if (e != NULL)
        {
          struct job_elem *job = list_entry (e, struct job_elem, elem);

          // Get the header info
          tjDecompressHeader3 (handle, job->buf, job->fileSize,
            &width, &height, &jpegSubsamp, &jpegColorspace);

          // Decompress the jpeg into RGB
          tjDecompress2 (handle, job->buf, job->fileSize, raw,
            width, 0, height, TJPF_RGB, 0);

          // Write the RGB file
          mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
          int outfd = open (job->fullName,
            O_WRONLY | O_CREAT | O_TRUNC, mode);
          write (outfd, raw, width * height * tjPixelSize[TJPF_RGB]);
          close (outfd);

          // Free memory
          free (job->buf);
          free (job);
        }
      e = atomic_queue_pop (&queue);
    }
  free (raw);
  tjDestroy (handle);
  return NULL;
}

int
main (int agrc, char **argv)
{
  if (agrc != 4 && agrc != 5)
    {
      usage ();
      return 1;
    }
  const char *directory = argv[1];
  unsigned long threads = strtoul (argv[2], NULL, 10);
  const char *port = argv[3];
  const char *address = NULL;
  if (agrc == 5)
    address = argv[4];

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

  atomic_queue_init (&queue);
  pthread_t *workers = malloc (threads * sizeof *workers);

  // Notify the server
  status = 10;
  send (new_fd, &status, sizeof status, MSG_EOR);

  off_t fileSize;
  ssize_t jobSize = 0;
  int fileNum = 1;
  status = retrieve (new_fd, &fileSize, sizeof fileSize);
  for (unsigned long i = 0; i < threads * 2; i++)
    {
      printf ("File size: %ld\n", fileSize);
      struct job_elem *job = malloc (sizeof *job);
      job->buf = malloc (fileSize);
      status = retrieve (new_fd, job->buf, fileSize);
      jobSize += fileSize;
      job->fileSize = fileSize;
      snprintf (job->fullName, 300, "%s/%d.raw", directory, fileNum);
      fileNum++;

      atomic_queue_push (&queue, &job->elem);

      // Get the next file size.
      status = retrieve (new_fd, &fileSize, sizeof fileSize);
    }

  for (unsigned long i = 1; i < threads; i++)
    pthread_create (&workers[i], NULL, thread_decompress, NULL);

  while (fileSize != 0)
    {
      printf ("File size: %ld\n", fileSize);
      struct job_elem *job = malloc (sizeof *job);
      job->buf = malloc (fileSize);
      status = retrieve (new_fd, job->buf, fileSize);
      jobSize += fileSize;
      job->fileSize = fileSize;
      snprintf (job->fullName, 300, "%s/%d.raw", directory, fileNum);
      fileNum++;

      atomic_queue_push (&queue, &job->elem);

      // Get the next file size.
      status = retrieve (new_fd, &fileSize, sizeof fileSize);
    }

  pthread_create (workers, NULL, thread_decompress, NULL);

  transmit = false;

  for (unsigned long i = 0; i < threads; i++)
    pthread_join (workers[i], NULL);

  // Notify the server
  send (new_fd, &status, sizeof status, MSG_EOR);

  close (new_fd);
  if (address == NULL)
    close (sockfd);
  freeaddrinfo (res);
  free (workers);

  return 0;

 error:
  fprintf(stderr, "Error! %d\n", errno);
  return 1;
}
