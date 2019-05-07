#ifndef __ATOMIC_QUEUE
#define __ATOMIC_QUEUE

#include <pthread.h>
#include "list.h"

struct atomic_queue
  {
    struct list queue;
    pthread_mutex_t mutex;
  };

void atomic_queue_init (struct atomic_queue *);

struct list_elem *atomic_queue_front (struct atomic_queue *);

struct list_elem *atomic_queue_back (struct atomic_queue *);

void atomic_queue_push (struct atomic_queue *, struct list_elem *);

struct list_elem * atomic_queue_pop (struct atomic_queue *);

bool atomic_queue_empty (struct atomic_queue *);

#endif /* atomic_queue.h */
