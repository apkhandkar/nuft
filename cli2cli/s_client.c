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
#include "downloadman.h"

int 
send_file(int port, char *fname, char *my_ident, char *to_ident) 
{

	fd_set recvset;
	FD_ZERO(&recvset);
	int are_ready; 

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

	if(sendto(	
		sockfd,
		init_mesg,
		sizeof(struct msg),
		0,
		(const struct sockaddr*)&servaddr,
		len) < 0) {

		perror("sendto - init message");
		return -1;

	}

	struct msg *recv_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *list_mesg = (struct msg*) malloc(sizeof(struct msg));
	struct msg *send_mesg = (struct msg*) malloc(sizeof(struct msg));

	(*list_mesg).type = 1;
	sprintf((*list_mesg).ids, "%s", my_ident);

	struct timeval timeout;

	FD_SET(sockfd, &recvset);

	/* to see how efficient this protocol is, */
	int listens_sent = 0;	/* number of listen messages send */
	int listens_disc = 0;	/* number of listens for which we got no response */

	while(1) {

		/**
		 * Do not trust select() to keep the contents of timeout intact.
		 * On Linux, it seems to modify timeout to reflect the amount of
		 * time not slept; on Mac OSX/BSD it keeps timeout unchanged but 
		 * advises programmers not to rely on that fact...!!!
		 */
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;

		/* listen for messages that are addressed to us */
		if(sendto(	
			sockfd,
			list_mesg,
			sizeof(struct msg),
			0,
			(const struct sockaddr*)&servaddr,
			len) < 0) {

			perror("sendto - listen message");
			return -1;

		}

		listens_sent += 1;

		/**
		 * The way the client and server communicate with each other 
		 * means if recvfrom() blocks, it means the server didn't respond
		 * to the listen message we sent, and it will block indefinitely.
		 * Thus, we have to ensure that recvfrom() NEVER blocks.
		 * 
		 * We poll the socket fd using select() to ensure recvfrom() is 
		 * only called when it won't block. The timeout is 100us. If
		 * sockfd isn't ready to recvfrom without blocking within this 
	 	 * interval, we resend the listen message.
		 *
		 * Announcing our presence using listen messages is necessary 
		 * because the server isn't going to remember our address.
		 * 
		 * Reducing the timeout below 100us could flood the server with 
		 * an excessive amount of listen messages.
		 */
		/**
		 * Time taken to transfer a ~945MiB file between two clients via
		 * relay server on the same device (when the server is only 
		 * dealing with this one file):
		 * 
		 *	select() waits 250us: ~ 00:07:30 (unacceptable)	
		 * 	select() waits 100us: ~ 00:04:01 (somewhat okay)
		 */
		if((are_ready = select((sockfd+1), &recvset, NULL, NULL, &timeout)) < 0) {

			perror("select");
			return -1;

		} else if(are_ready == 0) {

			/* no response, ping again... */
			/* put sockfd back in our fdset */
			FD_SET(sockfd, &recvset);

			/* the listen we sent didn't get a response */
			listens_disc += 1;

		} else {

			/* an fd from our fdset is ready to be read from */
			/* although sockfd is its only member, check to be sure */
			if(FD_ISSET(sockfd, &recvset)) {
	
				n = recvfrom(	
					sockfd, 
					recv_mesg, 
					sizeof(struct msg), 
					0, 
					(struct sockaddr*)&servaddr, 
					&len);	

				if((*recv_mesg).type == 2) {

					/* previous messsage was acknowledged */
					/* next block is being requested */

					/* set seek to the start of the requested block */
					lseek(fd, (((*recv_mesg).blkno - 1) * 1024), SEEK_SET);

					/* build message */
					(*send_mesg).type = 3;
					(*send_mesg).blkno = (*recv_mesg).blkno;
					sprintf((*send_mesg).ids, "%s %s", my_ident, to_ident);
					read(fd, (*send_mesg).body, 1024);
					
				} else if((*recv_mesg).type == 4) {

					/* file transfer completed */
					printf("File transfer completed.\n");
					close(fd);

					/* print statistics */
					printf("'Listen' messages sent: %d\n", listens_sent);
					printf("'Listen' messages wasted: %d\n", listens_disc);

					return 0;

				}

				/* send response */
				if(sendto(	
					sockfd, 
					send_mesg, 
					sizeof(struct msg), 
					0, 
					(struct sockaddr*)&servaddr, 
					len) < 0) {

					perror("sendto");
					return -1;

				}

			}

		}

	}
	
	close(fd);

	return 0;
}

int
receive_files(int port, char *ident)
{

	fd_set recvset;
	FD_ZERO(&recvset);
	int are_ready; 

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
	int fd;

	struct timeval timeout;

	FD_SET(sockfd, &recvset);

	int dman_stat;

	while(1) {

		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
	
		if(sendto(	
			sockfd, 
			list_mesg, 
			sizeof(struct msg), 
			0, 
			(const struct sockaddr*)&servaddr, 
			len) < 0) {

			perror("sendto");
			return -1;

		}

		if((are_ready = select((sockfd+1), &recvset, NULL, NULL, &timeout)) < 0) {
			
			perror("select");
			return -1;

		} else if(are_ready == 0) {

			FD_SET(sockfd, &recvset);

		} else {

			if(FD_ISSET(sockfd, &recvset)) {

				n = recvfrom(	
					sockfd, 
					recv_mesg, 
					sizeof(struct msg), 
					0, 
					(struct sockaddr*)&servaddr, 
					&len);	

				/* save the identity of the fellow sending us the file */
				sscanf((*recv_mesg).ids, "%s %s", &sender_ident, &my_ident);

				if((*recv_mesg).type == 0) {

					/* save the details of the file */
					nblk = (*recv_mesg).blkno;
					sscanf((*recv_mesg).body, "%d %s", &lblk, &fname);
	
					printf("***INCOMING: %s is sending us '%s' (~%dKiB)\n", 
						sender_ident, 
						fname, 
						nblk);

					/* create a new download */
					if((dman_stat = new_download(sender_ident, fname, nblk, lblk, 1)) < 0) {
			
						printf("dman failed to create new download\n");
						return -1;
				
					}	
	
					/* send acknowledgement to the sender */
					/* i.e., ask for block #1 of the file */
					/* I'm inventing this 'protocol' as I go along (._.') */
					(*send_mesg).type = 2;
			
				} else if((*recv_mesg).type == 3) {

					/* check if the block was the requested block */
					if((*recv_mesg).blkno == get_blkno(sender_ident)) {
	
						if(get_blkno(sender_ident) == get_nblk(sender_ident)) {
	
							/* write the last block and wrap up */
							write(get_fd(sender_ident), (*recv_mesg).body, get_lblk(sender_ident));
					
							/* file transfer finished */
							(*send_mesg).type = 4;
							sprintf((*send_mesg).body, "%s", get_fname(sender_ident));
	
						} else {
				
							/* write the block, ask for next block, etc. */
							write(get_fd(sender_ident), (*recv_mesg).body, 1024);
					
							inc_blkno(sender_ident);

							(*send_mesg).type = 2;

						}

					} else {
				
						/* ask for the same block again */
						(*send_mesg).type = 2;
					}
					
				}

				/* build response */
				(*send_mesg).blkno = get_blkno(sender_ident);
				sprintf((*send_mesg).ids, "%s %s", ident, sender_ident);

				/* send response */
				if(sendto(	
					sockfd, 
					send_mesg, 
					sizeof(struct msg), 
					0, 
					(const struct sockaddr*)&servaddr, 
					len) < 0) {

					perror("sendto");
					return -1;

				}

				/* if file was sent, we're done */
				if((*send_mesg).type == 4) {

					printf("Finished transfer.\n");

					finish_download(sender_ident);	
					
					//return 0;

				}

			}

		}

	}

	return 0;
}

int
main(int argc, char **argv)
{

	if(!strcmp(argv[1], "-s") && argc == 6) {

		if(strlen(argv[3]) > 512 || strlen(argv[4]) > 63 || strlen(argv[5]) > 63) {
		
			printf("note: IDs can't be longer than 63 characters\n");
			printf("also: Filenames can't be longer than 512 characters\n");
			return -1;

		}

		send_file(atoi(argv[2]), argv[3], argv[4], argv[5]);

	} else if(!strcmp(argv[1], "-r") && argc == 4) {

		if(strlen(argv[3]) > 63) {
		
			printf("note: id's can be up to 63 characters long\n");
			return -1;

		}

		receive_files(atoi(argv[2]), argv[3]);

	} else {

		printf("usage: client -s|-r <port> {<filename> <from> <to> | <identity>}\n");
		return 0;

	}

	return 0;			
}