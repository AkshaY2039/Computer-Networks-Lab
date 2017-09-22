CC = gcc
CFLAGS = -ansi -Wall -D_POSIX_C_SOURCE
COMMON = file.c fsm.c netudp.c tftp.c

all: client server

server:
	$(CC) $(CFLAGS) server.c $(COMMON) -o server
client: 
	$(CC) $(CFLAGS) client.c $(COMMON) -o client

clean:
	rm client
	rm server
    
