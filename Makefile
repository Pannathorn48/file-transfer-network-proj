# Define the C compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I./headers
LDFLAGS = 

# Define object files for each program to improve clarity and reduce redundancy
SERVER_OBJS = ./bin/server.o ./bin/server_connection.o ./bin/message.o ./bin/checksum.o ./bin/request_send_file.o
CLIENT_OBJS = ./bin/client.o ./bin/client_connection.o ./bin/message.o ./bin/checksum.o ./bin/request_send_file.o
TEST_OBJS = ./bin/checksum_test.o ./bin/checksum.o ./bin/message.o # Group test objects for clarity

# ---
# General Targets and Setup
# ---

.PHONY: all build clean test checksum_test mkdir_bin clear clear-o
all: build

# 'build' target relies on 'mkdir_bin' and then builds the executables
build: mkdir_bin server client

# New rule to create the bin directory - idempotent
mkdir_bin:
	@mkdir -p ./bin

# ---
# Executable Targets
# ---

server: $(SERVER_OBJS)
	@echo "Linking server executable"
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o ./bin/server $(LDFLAGS)

client: $(CLIENT_OBJS)
	@echo "Linking client executable"
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o ./bin/client $(LDFLAGS)

checksum_test: $(TEST_OBJS)
	@echo "Linking checksum test"
	$(CC) $(CFLAGS) $^ -o ./bin/checksum_test $(LDFLAGS)

test: checksum_test
	@echo "Running tests..."
	@./bin/checksum_test

# ---
# Object File Compilation Rules (Static Rules with Directory Prerequisite)
# The '| mkdir_bin' ensures the ./bin directory exists before compiling.
# ---

./bin/server.o: ./cores/server/main.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/server_connection.o: ./cores/server/connection.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/client.o: ./cores/client/main.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/client_connection.o: ./cores/client/connection.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/message.o: ./cores/common/message.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/checksum.o: ./cores/common/checksum.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/request_send_file.o: ./cores/common/request_send_file.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

./bin/checksum_test.o: ./tests/checksum_test.c | mkdir_bin
	$(CC) $(CFLAGS) -c $< -o $@

# ---
# Cleanup Targets
# ---

# Clears all contents of the bin directory
clear:
	@echo "Clearing ./bin directory"
	@rm -rf ./bin/*

# 'clean' usually removes all build artifacts, often synonym for 'clear'
clean: clear

# Target to specifically remove only object files, if needed
clear-o:
	@echo "Clearing object files"
	@rm -f ./bin/*.o