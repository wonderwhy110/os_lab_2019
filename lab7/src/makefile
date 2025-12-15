CC = gcc
CFLAGS = -Wall -Wextra -std=c99

TCP_CLIENT = tcpclient
TCP_SERVER = tcpserver
UDP_CLIENT = udpclient
UDP_SERVER = udpserver

all: $(TCP_CLIENT) $(TCP_SERVER) $(UDP_CLIENT) $(UDP_SERVER)

# TCP клиент
$(TCP_CLIENT): tcpclient.c
	$(CC) $(CFLAGS) -o $(TCP_CLIENT) tcpclient.c

# TCP сервер
$(TCP_SERVER): tcpserver.c
	$(CC) $(CFLAGS) -o $(TCP_SERVER) tcpserver.c

# UDP клиент
$(UDP_CLIENT): udpclient.c
	$(CC) $(CFLAGS) -o $(UDP_CLIENT) udpclient.c

# UDP сервер
$(UDP_SERVER): udpserver.c
	$(CC) $(CFLAGS) -o $(UDP_SERVER) udpserver.c

clean:
	rm -f $(TCP_CLIENT) $(TCP_SERVER) $(UDP_CLIENT) $(UDP_SERVER)

.PHONY: all clean
