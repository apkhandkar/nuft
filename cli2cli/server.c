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

	printf("Started relay server at port %d\n", port);
	
	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));

	int len;
	int n;
	int msgq_index;

	char id[64];

	while(1) {
	
		len = sizeof(cliaddr);

		n = recvfrom(	sockfd, 
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
			 * ID |	Type			  |	From   | To
			 * ---+-------------------+--------+-------
			 * 0  |	'newfile'		  |	Client | Client
			 * 2  |	'acknowledgement' |	Client | Client
			 * 3  |	file block		  |	Client | Client
			 * 4  |	'completed'		  |	Client | Client
			 *  
			 * Our job as the server is to simply add them to the
			 * message queue.
			 *
			 * A client-to-client message will [roughly] follow
			 * this path:
			 * 
			 *	Client [Sender] -->
			 *		Server =(adds to)=>
			 * 			Server Message Queue 
		  	 *				=(retrieved from)=> Server -->
			 * 					Client [Recipient]
			 */
			add_to_queue(recv_mesg);

			/**
			 * Do a bit of snooping around to print status messages
			 * 
			 * Messages that announce new transfers (type 0) are 
			 * checked for.
			 */
			if((*recv_mesg).type == 0) {
				printf("\nA new transfer is starting...\n");
			}

		} else if((*recv_mesg).type == 1) {

			/**
			 * A client has sent us a 'listening' message.
			 * With it, he will send his identity. We have to check
			 * if our queue contains any messages addressed to him,
			 * and if so, we send him the first message in the queue
			 * for him. 
			 *
			 * ID |	Type			  |	From   | To
			 * ---+-------------------+--------+-------
			 * 1  |	'listening'		  |	Client | Server
			 * 
			 * If there are no messages for the client, we do not 
			 * send any response.
			 */

			/* retrieve client identity */
			sscanf((*recv_mesg).ids, "%s", id);
	
			/* get the first message from the queue addressed to the client */
			/* if such a message exists, send it to the client */
			if((msgq_index = get_mqindex_to(id)) >= 0) {

				if(sendto(	sockfd, 
							get_message(msgq_index),	/* retrieve the message from queue */ 
							sizeof(struct msg), 
							0, 
							(struct sockaddr*)&cliaddr, 
							len) < 0) {

					perror("sendto");
					return -1;

				}

				/* delete the message we just sent from the message queue */
				delete_from_queue(msgq_index);

			} else {

				/* there were no messages addressed to the client in the queue */
				/* do nothing */

			}

		}
		
	}
	
	return 0;	
}
