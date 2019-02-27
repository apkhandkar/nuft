#ifndef CLI2CLI_H
#define CLI2CLI_H

struct msg {
	short int type;
	int blkno;
	char ids[129];
	char body[1024];
};

struct ident {
	char addr[16];
	int port;
};

#endif
