#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "cli2cli.h"
#include "message_queue.h"
#include "printmsg.h"

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

	printf("started relay server at port %d\n", port);
	
	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));

	int len;
	int n;
	int msgq_index;

	char id[64];

	/* start serving client requests */
	while(1) {
	
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, recv_mesg, sizeof(struct msg), 0, (struct sockaddr*)&cliaddr, (socklen_t*)&len);

		printf("---got a message from client---\n");

		if(	(*recv_mesg).type == 0 || (*recv_mesg).type == 2 || 
			(*recv_mesg).type == 3 || (*recv_mesg).type == 4 ){
			add_to_queue(recv_mesg);
			print_queue(0);
		} else if((*recv_mesg).type == 1) {
			/* a client has informed us he's listening for files */

			printmsg(recv_mesg);

			/* look for messages addressed to this client */
			sscanf((*recv_mesg).ids, "%s", id);

			if((msgq_index = get_mqindex_to(id)) >= 0) {
				if(sendto(sockfd, get_message(msgq_index), sizeof(struct msg), 0, (struct sockaddr*)&cliaddr, len) < 0) {
					perror("sendto");
					return -1;
				}
			}

			/* delete the message we just sent from the message queue */
			delete_from_queue(msgq_index);
			
			printf("---deleted the message just relayed...\n");
			print_queue(0);
		}
		
		printf("\n");
	}
	
	return 0;	
}
