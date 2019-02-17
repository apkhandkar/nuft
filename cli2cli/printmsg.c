#include <stdio.h>
#include "cli2cli.h"

/**
 * A function to help with debugging
 * Prints the contents of a cli2cli message
 */
void
printmsg(struct msg *mesg)
{
	int lblk;
	char id1[64], id2[64];
	char fname[256];
	
	switch((*mesg).type) {
		case 0:
			/* 'newfile' message */
			printf("Type: 'newfile'\n");
			sscanf(((*mesg).ids), "%s %s", &id1, &id2);
			sscanf(((*mesg).body), "%d %s", &lblk, &fname);
			printf("Filename: %s\n", fname);
			printf("From: %s\n", id1);
			printf("To: %s\n", id2);
			printf("Number of 1K blocks: %d\n", (*mesg).blkno);
			printf("Size of last block: %d bytes\n", lblk);
			break;
		case 1:
			/* 'listening' message */
			printf("Type: 'listening'\n");
			sscanf(((*mesg).ids), "%s", &id1);
			printf("Identity: %s\n", id1);
			break;
		default:
			break;
	}
} 
