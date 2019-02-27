#include <math.h>
#include <stdio.h>
#include <fcntl.h>
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
	if(inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) < 0) {

		printf("Invalid address\n");
		return -1;

	}
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

	while(1) {

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

			return 0;

		} else if((*recv_mesg).type == 6) {
	
			/* there was nothing for us, send a listen message again */
			continue;
	
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
	if(inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) < 0) {

		printf("Invalid address\n");
		return -1;

	}
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

	int dman_stat;

	while(1) {

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
					
		} else if((*recv_mesg).type == 6) {

			continue;			

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
