CC = gcc
CFLAGS = -Wall -Wextra -I./headers

build: clear server client clear-o
server: server_connection.o server.o  message.o
	@echo "Compiling server"
	$(CC) $(CFLAGS) ./bin/server.o ./bin/server_connection.o ./bin/message.o -o ./bin/server
client: client_connection.o client.o  message.o
	@echo "Compiling client"
	$(CC) $(CFLAGS) ./bin/client.o ./bin/message.o ./bin/client_connection.o -o ./bin/client
debug.o:
	$(CC) $(CFLAGS) ./cores/common/debug.c -c -o ./bin/debug.o
# request_send_file.o:
# 	$(CC) $(CFLAGS) ./cores/common/request_send_file.c -c -o ./bin/request_send_file.o
server_connection.o:
	$(CC) $(CFLAGS) ./cores/server/connection.c -c -o ./bin/server_connection.o
client_connection.o:
	$(CC) $(CFLAGS) ./cores/client/connection.c -c -o ./bin/client_connection.o
server.o:
	$(CC) $(CFLAGS) ./cores/server/main.c -c -o ./bin/server.o
client.o:
	$(CC) $(CFLAGS) ./cores/client/main.c -c -o ./bin/client.o
message.o:
	$(CC) $(CFLAGS) ./cores/common/message.c -c -o ./bin/message.o
clear:
	@echo "clear /bin"
	@rm -rf ./bin/*
clear-o:
	@echo "clear .o files"
	@rm -rf ./bin/*.o
