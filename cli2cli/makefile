cli2cli: client server

client: client.c printmsg.o cli2cli.h printmsg.h
	$(CC) client.c printmsg.o -lm -o client

server: server.c printmsg.o message_queue.o message_queue.h cli2cli.h printmsg.h 
	$(CC) server.c printmsg.o message_queue.o -o server

mq_test: message_queue_test.c message_queue.h message_queue.o printmsg.o printmsg.h
	$(CC) message_queue_test.c message_queue.o printmsg.o -o mq_test

message_queue.o : message_queue.c message_queue.h printmsg.o cli2cli.h printmsg.h 
	$(CC) -c message_queue.c

printmsg.o: printmsg.c cli2cli.h
	$(CC) -c printmsg.c

smsgqueue.o: smsgqueue.c cli2cli.h printmsg.h smsgqueue.h
	$(CC) -c smsgqueue.c

