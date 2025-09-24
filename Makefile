# Define the C compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I./headers
LDFLAGS = 

# Define object files for each program to improve clarity and reduce redundancy
SERVER_OBJS = ./bin/server.o ./bin/server_connection.o ./bin/message.o ./bin/checksum.o ./bin/request_send_file.o
CLIENT_OBJS = ./bin/client.o ./bin/client_connection.o ./bin/message.o ./bin/checksum.o ./bin/request_send_file.o

# Define common build and clean rules
.PHONY: all build clean test checksum_test
all: build

build: server client

# Main targets for building the server and client executables
server: $(SERVER_OBJS)
	@echo "Compiling server"
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o ./bin/server $(LDFLAGS)

client: $(CLIENT_OBJS)
	@echo "Compiling client"
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o ./bin/client $(LDFLAGS)

# Rules for compiling individual object files with automatic dependency handling
# Use automatic variables ($< for the prerequisite, $@ for the target) for cleaner rules
./bin/server.o: ./cores/server/main.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/server_connection.o: ./cores/server/connection.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/client.o: ./cores/client/main.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/client_connection.o: ./cores/client/connection.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/message.o: ./cores/common/message.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/checksum.o: ./cores/common/checksum.c
	$(CC) $(CFLAGS) -c $< -o $@

./bin/request_send_file.o: ./cores/common/request_send_file.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test target for checksum_test. It also needs the message and checksum object files.
./bin/checksum_test.o: ./tests/checksum_test.c
	$(CC) $(CFLAGS) -c $< -o $@

checksum_test: ./bin/checksum_test.o ./bin/checksum.o ./bin/message.o
	@echo "Compiling checksum test"
	$(CC) $(CFLAGS) $^ -o ./bin/checksum_test $(LDFLAGS)

test: checksum_test
	@echo "Running tests..."
	@./bin/checksum_test

# Cleanup targets
clear:
	@echo "clear /bin"
	@rm -rf ./bin/*

clean: clear

.PHONY: clear-o
clear-o:
	@echo "clear object files"
	@rm -f ./bin/*.o