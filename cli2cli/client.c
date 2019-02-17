#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "cli2cli.h"
#include "printmsg.h"

int 
send_file(int port, char *fname, char *my_ident, char *to_ident) 
{

	/*******/
	fd_set recvset;
	FD_ZERO(&recvset);
	int are_ready; 
	/*******/

	printf("Sending file: %s\n", fname);
	printf("via relayserver @ port %d\n", port);
	printf("To: %s\n", to_ident);
	printf("From: %s\n", my_ident);

	int sockfd, len;
	int n;
	
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

	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *list_mesg = (struct msg*) malloc(sizeof(struct msg));
	(*list_mesg).type = 1;
	sprintf((*list_mesg).ids, "%s", my_ident);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 250;

	while(1) {

		

		/* do not trust select() to keep contents of timeout unchanged */
		timeout.tv_sec = 0;
		timeout.tv_usec = 250;

		/* listen for messages that are addressed to us */
		if(sendto(sockfd, list_mesg, sizeof(struct msg), 0, (const struct sockaddr*)&servaddr, len) < 0) {
			perror("sendto - listen message");
			return -1;
		}

		/* ping server for a response from recipient */
		are_ready = select(sockfd+1, &recvset, NULL, NULL, &timeout);
		
		if(are_ready == 0) {

			/* no response, keep waiting... */
			/* put sockfd back in our fdset */
			FD_SET(sockfd, &recvset);

		} else if(are_ready == -1) {

			perror("select");
			return -1;

		} else {

			/* an fd from our fdset is ready to be read from */
			/* although sockfd is its only member, check to be sure */
			if(FD_ISSET(sockfd, &recvset)) {
	
				printf("got reply\n");
				n = recvfrom(sockfd, recv_mesg, sizeof(struct msg), 0, (struct sockaddr*)&servaddr, &len);	

				if((*recv_mesg).type == 2) {

					/* previous messsage was acknowledged */
					/* next block is being requested */

					printf("Was acknowledged...\n");
				}

				return 0;
			}
		}


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
	struct msg *list_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *send_mesg = (struct msg*) malloc(sizeof(struct msg));

	(*list_mesg).type = 1;
	sprintf((*list_mesg).ids, "%s", ident);

	char sender_ident[64];
	char my_ident[64];
	char fname[1024];
	int nblk, lblk;
	int blkno;

	while(1) {
	
		/* listen for files... */
		if(sendto(sockfd, list_mesg, sizeof(struct msg), 0, (const struct sockaddr*)&servaddr, len) < 0) {
			perror("sendto");
			return -1;
		}

		n = recvfrom(sockfd, recv_mesg, sizeof(struct msg), 0, (struct sockaddr*)&servaddr, &len);	

		if((*recv_mesg).type == 0) {
			blkno = 1;	

			/* save the identity of the fellow sending us the file */
			sscanf((*recv_mesg).ids, "%s %s", &sender_ident, &my_ident);
			/* save the details of the file */
			nblk = (*recv_mesg).blkno;
			sscanf((*recv_mesg).body, "%d %s", &lblk, &fname);
	
			printf("***INCOMING: %s is sending us '%s' (~%dKiB)\n", sender_ident, fname, nblk);

			/* send acknowledgement to the sender */
			/* i.e., ask for block #1 of the file */
			/* I'm inventing this 'protocol' as I go along (._.') */
			(*send_mesg).type = 2;
			(*send_mesg).blkno = blkno;
			sprintf((*send_mesg).ids, "%s %s", ident, sender_ident);
			
		}

		/* send response */
		if(sendto(sockfd, send_mesg, sizeof(struct msg), 0, (const struct sockaddr*)&servaddr, len) < 0) {
			perror("sendto");
			return -1;
		}
	
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
