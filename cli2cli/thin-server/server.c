#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "cli2cli.h"

int 
main(int argc, char **argv)
{

	/* default port is 8080 */
	int port = 8080;
	/* unless specified by the user as a command-line argument */
	if(argc == 2) port = atoi(argv[1]);

	int sockfd;

	struct sockaddr_in servaddr, cliaddr;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {

		perror("socket");
		return -1;

	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {

		perror("bind");
		return -1;

	}

	printf("Started relay server at port %d\n\n", port);
	
	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *send_mesg = (struct msg*) malloc(sizeof(struct msg));
	
	struct msg *temp_mesg = (struct msg*) malloc(sizeof(struct msg));

	int len;
	int n;
	int msgq_index;

	char id1[64], id2[64], fname[512];
	char temp_from[64], temp_to[64];
	int lblk;

	while(1) {
	
		len = sizeof(cliaddr);

		n = recvfrom(	
			sockfd, 
			recv_mesg, 
			sizeof(struct msg), 
			0, 
			(struct sockaddr*)&cliaddr, 
			(socklen_t*)&len);

		if(	(*recv_mesg).type == 0 || (*recv_mesg).type == 2 || 
			(*recv_mesg).type == 3 || (*recv_mesg).type == 4 ){

			/**
			 * These are client-to-client messages.
			 * 
			 * Client [Sender] -->
			 * 		Server -->
			 * 			Client [Recipient]
			 */

			/* save message temporarily */
			memcpy(temp_mesg, recv_mesg, sizeof(struct msg));

			/* announce new transfers */
			if((*recv_mesg).type == 0) {

				sscanf((*recv_mesg).ids, "%s %s", &id1, &id2);
				sscanf((*recv_mesg).body, "%d %s", &lblk, &fname);

				printf("     [New Transfer] from:%s to:%s file:%s size:~%dKiB\n",
					id1,
					id2,
					fname,
					(*recv_mesg).blkno);

			}

		} else if((*recv_mesg).type == 1) {

			/* retrieve client identity */
			sscanf((*recv_mesg).ids, "%s", id1);

			/* check if the message we are storing is addressed to this client */
			sscanf((*temp_mesg).ids, "%s %s", temp_from, temp_to);

			if(!strcmp(temp_to, id1)) {

				/* send this message to the client */
				if(sendto(
					sockfd,
					temp_mesg,
					sizeof(struct msg),
					0,
					(struct sockaddr*)&cliaddr,
					len) < 0) {

					perror("sendto");
					return -1;

				}

				/* announce completed transfers */
				if((*temp_mesg).type == 4) {

					sscanf((*temp_mesg).ids, "%s %s", id1, id2);
					sscanf((*temp_mesg).body, "%s", fname);
		
					printf("[Finished Transfer] from:%s to:%s file:%s\n", id2, id1, fname);

				}

			} else {

				/* there were no messages addressed to the client in the queue */
				/* respond to the client with a 'nothing for you' message */
				(*send_mesg).type = 6;
				
				if(sendto(	
					sockfd, 
					send_mesg,	
					sizeof(struct msg), 
					0, 
					(struct sockaddr*)&cliaddr, 
					len) < 0) {

					perror("sendto");
					return -1;

				}

			} 

		} 
		
	}
	
	return 0;	

}
