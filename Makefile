

http-server: utils.c http-server.c main.c -lpthread
	gcc -o $@ $^

.PHONY: run
run: http-server
	./http-server 8081
