CC=gcc
CFLAGS=-Wall -Wextra -std=gnu99 -g

all: server client client_thread

server: server.c
	${CC} ${CFLAGS} -o server server.c

client: client.c
	${CC} ${CFLAGS} -lturbojpeg -o client client.c

client_thread: client_thread.o list.o atomic_queue.o
	${CC} ${CFLAGS} -lturbojpeg -lpthread -o client_thread client_thread.o atomic_queue.o list.o

client_thread.o: client_thread.c
	${CC} ${CFLAGS} -c client_thread.c -o client_thread.o

list.o: list.c list.h
	${CC} ${CFLAGS} -c list.c -o list.o

atomic_queue.o: atomic_queue.c atomic_queue.h
	${CC} ${CFLAGS} -c atomic_queue.c -o atomic_queue.o

clean:
	rm -f server client client_thread *.o

.PHONY: clean
