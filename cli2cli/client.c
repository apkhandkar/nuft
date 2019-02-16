#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "cli2cli.h"
#include "printmsg.h"

int 
send_file(int port, char *fname, char *my_ident, char *to_ident) 
{
	printf("Sending file: %s\n", fname);
	printf("via relayserver @ port %d\n", port);
	printf("To: %s\n", to_ident);
	printf("From: %s\n", my_ident);

	int sockfd, len;
	
	struct sockaddr_in servaddr;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	len = sizeof(servaddr);

	int fd;

	if((fd = open(fname, O_RDONLY)) < 0) {
		printf("Couldn't open file: invalid filename/insufficient permissions?\n");
		printf("Quitting\n");
		return -1;
	}

	struct stat st;

	if(fstat(fd, &st) < 0) {
		printf("Couldn't retrieve file information\n");
		printf("Quitting\n");
		return -1;
	}

	/* number of 1K blocks file will be sent in */
	int nblk = ceil(((float)(st.st_size) / 1024));
	/* size of last block */
	int lblk = (st.st_size) - ((nblk-1) * 1024);

	struct msg *init_mesg = (struct msg*) malloc(sizeof(struct msg));

	/* compose the 'newfile' message */
	(*init_mesg).type = 0;
	(*init_mesg).blkno = nblk;
	sprintf(((*init_mesg).ids), "%s %s", my_ident, to_ident);
	sprintf(((*init_mesg).body), "%d %s", lblk, fname);

	if(sendto(sockfd, init_mesg, sizeof(struct msg), 0, (const struct sockaddr*)&servaddr, len) < 0) {
		perror("sendto - init message");
		return -1;
	}
	
	close(fd);

	return 0;
}

int
receive_files(int port, char *ident)
{
	printf("Listening for files via relayserver at port %d as @%s\n", port, ident);

	int sockfd, len, n;
	
	struct sockaddr_in servaddr;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	len = sizeof(servaddr);

	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *send_mesg = (struct msg*) malloc(sizeof(struct msg));

	(*send_mesg).type = 1;
	sprintf((*send_mesg).ids, "%s", ident);

	while(1) {
	
		/* listen for files... */
		if(sendto(sockfd, send_mesg, sizeof(struct msg), 0, (const struct sockaddr*)&servaddr, len) < 0) {
			perror("sendto");
			return -1;
		}

		n = recvfrom(sockfd, recv_mesg, sizeof(struct msg), 0, (struct sockaddr*)&servaddr, &len);	
	
		printmsg(recv_mesg);
	
		return 0;
	}

	return 0;
}

int
main(int argc, char **argv)
{

	if(!strcmp(argv[1], "-s") && argc == 6) {

		send_file(atoi(argv[2]), argv[3], argv[4], argv[5]);

	} else if(!strcmp(argv[1], "-r") && argc == 4) {

		receive_files(atoi(argv[2]), argv[3]);

	} else {

		printf("usage: client -s|-r <port> {<filename> <from> <to> | <identity>}\n");
		return 0;

	}

	return 0;			
}
