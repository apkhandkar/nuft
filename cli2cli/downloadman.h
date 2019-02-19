#ifndef DOWNLOADMAN_H
#define DOWNLOADMAN_H

struct download {
	int fd;
	int nblk;
	int lblk;
	int blkno;
	char from[63];
	char fname[512];
	struct download *next;
	struct download *prev;
};

int new_download(char *from, char *fname, int nblk, int lblk, int blkno);
int get_fd(char *from);
int get_nblk(char *from);
int get_lblk(char *from);
int get_blkno(char *from);
void inc_blkno(char *from);
char *get_fname(char *from);
int finish_download(char *from);
void show_downloads(int direction);

#endif
