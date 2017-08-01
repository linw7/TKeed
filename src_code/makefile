TKeed:main.o util.o epoll.o http.o http_parse.o http_request.o priority_queue.o rio.o timer.o threadpool.o 
	gcc *.o -o TKeed -lpthread

main.o:main.c
	gcc -c main.c

util.o:util.c util.h
	gcc -c util.c

epoll.o:epoll.c epoll.h
	gcc -c epoll.c

http.o:http.c http.h
	gcc -c http.c

http_parse.o:http_parse.c http_parse.h
	gcc -c http_parse.c

http_request.o:http_request.c http_request.h
	gcc -c http_request.c

priority_queue.o:priority_queue.c priority_queue.h
	gcc -c priority_queue.c

rio.o:rio.c rio.h
	gcc -c rio.c

timer.o:timer.c timer.h
	gcc -c timer.c

threadpool.o:threadpool.c threadpool.h
	gcc -c threadpool.c

clean:
	rm *.o TKeed
