cli2cli: client server

client: client.c cli2cli.h downloadman.o downloadman.h
	$(CC) client.c downloadman.o -lm -o client

downloadman.o: downloadman.c downloadman.h
	$(CC) -c downloadman.c

server: server.c message_queue.o message_queue.h cli2cli.h  
	$(CC) server.c message_queue.o -o server

message_queue.o : message_queue.c message_queue.h printmsg.o cli2cli.h printmsg.h 
	$(CC) -c message_queue.c
