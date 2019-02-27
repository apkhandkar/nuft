client: client.c cli2cli.h downloadman.o downloadman.h
	$(CC) client.c downloadman.o -lm -o client
	strip client

server: server.c cli2cli.h  
	$(CC) server.c -o server
	strip server

downloadman.o: downloadman.c downloadman.h
	$(CC) -c downloadman.c

message_queue.o : message_queue.c message_queue.h cli2cli.h 
	$(CC) -c message_queue.c
