CC = gcc
CFLAGS = -Wall -Wextra -I./headers

build: clear server client
server: server_handler.o server.o
	@echo "Compiling server"
	$(CC) $(CFLAGS) ./bin/server.o ./bin/server_handler.o -o ./bin/server
	@make clear-o
client: debug.o
	@echo "Compiling client"
	$(CC) $(CFLAGS) ./cores/client/main.c ./bin/debug.o -o ./bin/client
debug.o:
	$(CC) $(CFLAGS) ./cores/common/debug.c -c -o ./bin/debug.o
server_handler.o:
	$(CC) $(CFLAGS) ./cores/server/server_handler.c -c -o ./bin/server_handler.o
server.o:
	$(CC) $(CFLAGS) ./cores/server/main.c -c -o ./bin/server.o
clear:
	@echo "clear /bin"
	@rm -rf ./bin/*
clear-o:
	@echo "clear .o files"
	@rm -rf ./bin/*.o
