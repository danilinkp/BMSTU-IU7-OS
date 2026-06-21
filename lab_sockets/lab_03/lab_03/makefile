CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -lpthread

all: server_1 client

server_1: server_1.c common.h
	cc $(CFLAGS) -o server server_1.c $(LDFLAGS)

server_2: server_2.c common.h
	cc $(CFLAGS) -o server server_2.c $(LDFLAGS)

client: client.c common.h
	cc $(CFLAGS) -o client client.c

clean:
	rm -f server client *.log
