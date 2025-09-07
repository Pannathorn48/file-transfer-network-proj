build: clear
	gcc ./cores/main.c -o ./bin/main.out
clear:
	rm -rf ./bin/*