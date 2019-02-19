#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "downloadman.h"

int dm_initialised = 0;

struct download *HEAD, *TAIL;

int
new_download(char *from, char *fname, int nblk, int lblk, int blkno)
{

	int got_fd;

	if((got_fd = open(fname, O_CREAT|O_WRONLY, 0644)) < 0) {
		
		/* failed to create file */
		/* downloads ll won't be initialised */
		return got_fd;

	} else {
	
		if(!dm_initialised) {

			HEAD = (struct download*) malloc(sizeof(struct download));
	
			(*HEAD).fd = got_fd;
			(*HEAD).nblk = nblk;
			(*HEAD).lblk = lblk;
			(*HEAD).blkno = 1;
			strcpy((*HEAD).from, from);
			strcpy((*HEAD).fname, fname);

			(*HEAD).next = NULL;
			(*HEAD).prev = NULL;

			TAIL = HEAD;

			dm_initialised = 1;

		} else {

			struct download *temp;
			temp  = (struct download*) malloc(sizeof(struct download));

			(*temp).fd = got_fd;
			(*temp).nblk = nblk;
			(*temp).lblk = lblk;
			(*temp).blkno = 1;
			strcpy((*temp).from, from);
			strcpy((*temp).fname, fname);

			(*TAIL).next = temp;
			(*temp).next = NULL;
			(*temp).prev = TAIL;

			TAIL = temp;

		}	

		return got_fd;

	}

}

int
get_fd(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			return (*temp).fd;

		}

		temp = (*temp).next;	

	}

	return -1;

}

int
get_nblk(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			return (*temp).nblk;

		}

		temp = (*temp).next;	

	}

	return -1;

}

int
get_lblk(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			return (*temp).lblk;

		}

		temp = (*temp).next;	

	}

	return -1;

}

int
get_blkno(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			return (*temp).blkno;

		}

		temp = (*temp).next;	

	}

	return -1;

}

void
inc_blkno(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			(*temp).blkno += 1;

		}

		temp = (*temp).next;	

	}

}

char
*get_fname(char *from)
{

	struct download *temp = HEAD;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			return &((*temp).fname);

		}

		temp = (*temp).next;	

	}

	return -1;

}

int
finish_download(char *from)
{

	struct download *temp = HEAD;
	struct download *copy;

	while(temp != NULL) {

		if(!strcmp((*temp).from, from)) {

			close((*temp).fd);

			if(temp == HEAD) {

				if(HEAD == TAIL) {

					free(HEAD);
					HEAD = NULL;
					TAIL = NULL;
					dm_initialised = 0;

				} else {
	
					copy = HEAD;
					HEAD = (*HEAD).next;
					(*HEAD).prev = NULL;
					free(copy);

				}

			} else if(temp == TAIL) {

				copy = TAIL;
				TAIL = (*TAIL).prev;
				(*TAIL).next = NULL;
				free(copy);

			} else {

				(*((*temp).prev)).next = (*temp).next;
				(*((*temp).next)).prev = (*temp).prev;
				free(temp);

			}

			return 0;		

		}

		temp = (*temp).next;
	
	}

	return -1;

}

void
show_downloads(int direction)
{

	struct download *temp = direction ?
		TAIL :
		HEAD ;

	printf("\n-----Active Downloads List-----\n");

	while(temp != NULL) {

		printf("-----Download-----\n");
		printf("FD: %d\n", (*temp).fd);
		printf("From: %s\n", (*temp).from);
		printf("File: %s\n", (*temp).fname);
		printf("----/Download-----\n");
	
		temp = direction ?
			(*temp).prev :
			(*temp).next ;

	}
		
	printf("----/Active Downloads List-----\n\n");

}
