build: clear server client
	echo "Compiling Everything"
server:
	gcc ./cores/server/main.c -o ./bin/server
client:
	gcc ./cores/client/main.c -o ./bin/client
clear:
	rm -rf ./bin/*
