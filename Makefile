CC = gcc
CFLAGS = -Wall -Wextra -I./headers

build: clear server client
server: server_handler.o server.o request_send_file.o
	@echo "Compiling server"
	$(CC) $(CFLAGS) ./bin/server.o ./bin/server_handler.o ./bin/request_send_file.o -o ./bin/server
	@make clear-o
client: client.o request_send_file.o
	@echo "Compiling client"
	$(CC) $(CFLAGS) ./bin/client.o ./bin/request_send_file.o  -o ./bin/client
debug.o:
	$(CC) $(CFLAGS) ./cores/common/debug.c -c -o ./bin/debug.o
request_send_file.o:
	$(CC) $(CFLAGS) ./cores/common/request_send_file.c -c -o ./bin/request_send_file.o
server_handler.o:
	$(CC) $(CFLAGS) ./cores/server/server_handler.c -c -o ./bin/server_handler.o
server.o:
	$(CC) $(CFLAGS) ./cores/server/main.c -c -o ./bin/server.o
client.o:
	$(CC) $(CFLAGS) ./cores/client/main.c -c -o ./bin/client.o
clear:
	@echo "clear /bin"
	@rm -rf ./bin/*
clear-o:
	@echo "clear .o files"
	@rm -rf ./bin/*.o
