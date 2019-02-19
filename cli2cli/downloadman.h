#ifndef DOWNLOADMAN_H
#define DOWNLOADMAN_H

struct download {
	int fd;
	char from[63];
	char fname[512];
	struct download *next;
	struct download *prev;
};

int new_download(char *from, char *fname);
int get_fd(char *from);
int finish_download(char *from);
void show_downloads(int direction);

#endif
