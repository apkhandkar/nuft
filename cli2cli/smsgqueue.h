#ifndef SMSGQUEUE_H
#define SMSGQUEUE_H

#include "cli2cli.h"

struct mq_entry {
	struct msg mesg;
	struct mq_entry *next_entry;
};

int add_to_queue(struct msg *recv_mesg);
struct msg *getmsg_to(char *pto);
void print_msgq(void);

#endif
