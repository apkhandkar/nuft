#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "cli2cli.h"

struct mq_entry {
	struct msg message;
	struct mq_entry *next_entry;
	struct mq_entry *prev_entry;
};

int add_to_queue(struct msg *recv_mesg);
int get_mqindex_to(char *to_ident);
struct msg *get_message(int at_index);
int delete_from_queue(int at_index);
void print_queue(int direction);

#endif
