#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli2cli.h"
#include "smsgqueue.h"
#include "printmsg.h"

struct mq_entry *HEAD, *CURR;

int
add_to_queue(struct msg *recv_mesg)
{
	static int smsgq_init = 0;

	if(smsgq_init == 0) {

		HEAD = (struct mq_entry*) malloc(sizeof(struct mq_entry));
		
		(*HEAD).next_entry = NULL;

		memcpy(&((*HEAD).mesg), recv_mesg, sizeof(struct msg));

		CURR = HEAD;
	
		smsgq_init = 1;

		return 0;
	}

	struct mq_entry *temp = (struct mq_entry*) malloc(sizeof(struct mq_entry));
	
	(*CURR).next_entry = temp;
	(*temp).next_entry = NULL;
	
	memcpy(&((*temp).mesg), recv_mesg, sizeof(struct msg));

	CURR = temp;

	return 0;
}

struct msg
*getmsg_to(char *pto)
{
	struct mq_entry *temp = HEAD;
	char from[64], to[64];

	while(temp != NULL) {

		sscanf((*temp).mesg.ids, "%s %s", &from, &to);
		printf("to: %s pto: %s\n", to, pto);
		if(!strcmp(to, pto)) {
			return &((*temp).mesg);
		}

		temp = (*temp).next_entry;
	}
	return NULL;
}

void
print_msgq()
{
	struct mq_entry *temp = HEAD;

	printf("\n---printing server message queue---\n\n");
	
	while(temp != NULL) {
		
		printmsg(&((*temp).mesg));
		
		printf("\n");

		temp = (*temp).next_entry;
	}

	printf("\n--/printing server message queue---\n\n");
}
