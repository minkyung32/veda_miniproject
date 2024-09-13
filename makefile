OBJS = server.o client.o
CC = gcc
CFLAGS = -c
CROSSCC = aarch64-linux-gnu-gcc-12

server-arm: server.o
	$(CROSSCC) -o $@ server.o
client: client.o
	$(CC) -o $@ client.o
.c.o:
	$(CC) $(CFLAGS) $<
clean :
	rm $(OBJS) server-arm client