cli2cli: client server

client: client.c printmsg.o cli2cli.h printmsg.h
	$(CC) client.c printmsg.o -lm -o client

server: server.c printmsg.o smsgqueue.o cli2cli.h printmsg.h smsgqueue.h
	$(CC) server.c printmsg.o smsgqueue.o -o server

printmsg.o: printmsg.c cli2cli.h
	$(CC) -c printmsg.c

smsgqueue.o: smsgqueue.c cli2cli.h printmsg.h smsgqueue.h
	$(CC) -c smsgqueue.c

