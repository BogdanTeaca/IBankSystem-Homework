CC=gcc
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SERVER=server
CLIENT=client

build: $(SERVER) $(CLIENT)

$(SERVER):$(SERVER).c
	$(CC) -o $(SERVER) $(LIBSOCKET) $(SERVER).c

$(CLIENT):$(CLIENT).c
	$(CC) -o $(CLIENT) $(LIBSOCKET) $(CLIENT).c

clean:
	rm -f *.o *~
	rm -f $(SERVER) $(CLIENT)


