client: client.c cli2cli.h downloadman.o downloadman.h
	$(CC) client.c downloadman.o -lm -o client

server: server.c message_queue.o message_queue.h cli2cli.h  
	$(CC) server.c message_queue.o -o server

downloadman.o: downloadman.c downloadman.h
	$(CC) -c downloadman.c

message_queue.o : message_queue.c message_queue.h cli2cli.h 
	$(CC) -c message_queue.c
