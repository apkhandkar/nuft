#include "cli2cli.h"
#include "message_queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int queue_empty = 1;

struct mq_entry *HEAD, *TAIL;

int
add_to_queue(struct msg *recv_mesg)
{
	/* if the queue is empty */
	if(queue_empty) {

		/* create the first entry */
		HEAD = (struct mq_entry*) malloc(sizeof(struct mq_entry));

		(*HEAD).prev_entry = NULL;
		(*HEAD).next_entry = NULL;
		
		memcpy(&(*HEAD).message, recv_mesg, sizeof(struct msg));

		TAIL = HEAD;

		queue_empty = 0;
		return 0;

	}

	/* if the queue is non-empty */
	struct mq_entry *temp = (struct mq_entry*) malloc(sizeof(struct mq_entry));

	(*TAIL).next_entry = temp;
	(*temp).prev_entry = TAIL;
	(*temp).next_entry = NULL;
	
	memcpy(&(*temp).message, recv_mesg, sizeof(struct msg));

	/* set the new tail */
	TAIL = temp;

	return 0;

}

int
get_mqindex_to(char *to_ident)
{

	struct mq_entry *temp = HEAD;
	char from[64], to[64];
	int index = 0;

	while(temp != NULL) {

		sscanf((*temp).message.ids, "%s %s", &from, &to);

		if(!strcmp(to, to_ident)) {

			return index;

		}

		index += 1;
		temp = (*temp).next_entry;

	}
	
	return -1;

}

struct msg
*get_message(int at_index)
{

	struct mq_entry *temp = HEAD;
	int index = 0;

	while(temp != NULL) {

		if(index == at_index) {

			return &(*temp).message;

		}

		index += 1;
		temp = (*temp).next_entry;
	}
	
	return NULL;

}

int
delete_from_queue(int at_index)
{

	struct mq_entry *temp = HEAD, *copy;
	int index = 0;

	while(temp != NULL) {
		
		if(index == at_index) {

			if(temp == HEAD) {

				if(HEAD == TAIL) {

					/* only one element in the queue */
					free(HEAD);
					HEAD = NULL;
					TAIL = NULL;
					queue_empty = 1;					

				} else {
					
					/* gotta remove the first element */
					copy = HEAD;
					HEAD = (*HEAD).next_entry;
					(*HEAD).prev_entry = NULL;
					free(copy);
				}

			} else if(temp == TAIL) {

				/* gotta remove the last element */
				copy = TAIL;
				TAIL = (*TAIL).prev_entry;
				(*TAIL).next_entry = NULL;
				free(copy);

			} else {

				/* gotta remove this here element somewhere in the middle */
				(*((*temp).prev_entry)).next_entry = (*temp).next_entry;
				(*((*temp).next_entry)).prev_entry = (*temp).prev_entry;
				free(temp);

			}

			/* recalibrate the TAIL */
			temp = HEAD;
			while(temp != NULL) {

				TAIL = temp;
				temp = (*temp).next_entry;

			}	
			
			return 0;

		}

		index += 1;
		temp = (*temp).next_entry;
	}
	
	return -1;

}
