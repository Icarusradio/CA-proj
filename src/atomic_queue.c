#include "atomic_queue.h"
#include <assert.h>
#include <stdlib.h>

void
atomic_queue_init (struct atomic_queue *queue)
{
  assert (queue != NULL);
  list_init (&queue->queue);
  pthread_mutex_init (&queue->mutex, NULL);
}

struct list_elem *
atomic_queue_front (struct atomic_queue *queue)
{
  assert (queue != NULL);
  pthread_mutex_lock (&queue->mutex);
  struct list_elem *elem = list_begin (&queue->queue);
  pthread_mutex_unlock (&queue->mutex);
  return elem;
}

struct list_elem *
atomic_queue_back (struct atomic_queue *queue)
{
  assert (queue != NULL);
  pthread_mutex_lock (&queue->mutex);
  struct list_elem *elem = list_rbegin (&queue->queue);
  pthread_mutex_unlock (&queue->mutex);
  return elem;
}

void
atomic_queue_push (struct atomic_queue *queue, struct list_elem *elem)
{
  assert (queue != NULL);
  pthread_mutex_lock (&queue->mutex);
  list_push_back (&queue->queue, elem);
  pthread_mutex_unlock (&queue->mutex);
}

struct list_elem *
atomic_queue_pop (struct atomic_queue *queue)
{
  assert (queue != NULL);
  struct list_elem *elem;
  pthread_mutex_lock (&queue->mutex);
  if (list_empty (&queue->queue))
    elem = NULL;
  else
    elem = list_pop_front (&queue->queue);
  pthread_mutex_unlock (&queue->mutex);
  return elem;
}

bool
atomic_queue_empty (struct atomic_queue *queue)
{
  assert (queue != NULL);
  pthread_mutex_lock (&queue->mutex);
  bool result = list_empty (&queue->queue);
  pthread_mutex_unlock (&queue->mutex);
  return result;
}
