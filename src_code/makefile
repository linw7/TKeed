CC = gcc -std=gnu99

TKeed:main.o util.o epoll.o http.o http_parse.o http_request.o priority_queue.o rio.o timer.o threadpool.o 
	$(CC) *.o -o TKeed -lpthread

main.o:main.c
	$(CC) -c main.c

util.o:util.c util.h
	$(CC) -c util.c

epoll.o:epoll.c epoll.h
	$(CC) -c epoll.c

http.o:http.c http.h
	$(CC) -c http.c

http_parse.o:http_parse.c http_parse.h
	$(CC) -c http_parse.c

http_request.o:http_request.c http_request.h
	$(CC) -c http_request.c

priority_queue.o:priority_queue.c priority_queue.h
	$(CC) -c priority_queue.c

rio.o:rio.c rio.h
	$(CC) -c rio.c

timer.o:timer.c timer.h
	$(CC) -c timer.c

threadpool.o:threadpool.c threadpool.h
	$(CC) -c threadpool.c

clean:
	rm *.o TKeed
