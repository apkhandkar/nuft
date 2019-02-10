#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

struct fdreg {
  int fd;
  int actv;
  struct fdreg * next;
} *HEAD, *CURR;

int tlist_init = 0;

int
fd_getfor(char * fname)
{
  /*
   * The original plan was to assign a token number (integer) to every client
   * who asks for a file from the server.
   * The numbers would be assigned in an increasing order starting from 1, and
   * clients asking for the same file would be assigned the same token number.
   * When all clients assigned a particular token were served the file they
   * requested, the token number would be returned to a pool of unused token 
   * numbers. However, programming this 'pool' was proving to be arduous, and
   * besides, the same tokens were being assigned for the same file, so it was
   * decided to let the file descriptor play the role of token number, and the
   * 'token' terminology was [partially] retired. To the client it still is a
   * token, but to the server it is an FD shared amongst multiple clients
   */

  if(tlist_init == 0) { 
  
    HEAD = (struct fdreg*)malloc(sizeof(struct fdreg));
    
    HEAD->fd  = 0;
    HEAD->actv = 0;
    HEAD->next = NULL;

    CURR = HEAD;

    tlist_init = 1; 
  }

  struct stat st_fname, st_assigned;

  if(stat(fname, &st_fname) < 0) {
    return -1;
  }


  struct fdreg *temp = HEAD->next;
  while(temp != NULL) {
      
    if(fstat(temp->fd, &st_assigned) < 0) {
      return -1;
    }

    if(st_fname.st_ino == st_assigned.st_ino) {
      temp->actv += 1;
      return temp->fd;
    }

    temp = temp->next;
  }

  /* token for the file doesn't exist; create a new one */

  struct fdreg *new = (struct fdreg*)malloc(sizeof(struct fdreg));
  
  if((new->fd = open(fname, O_RDONLY)) < 0) return -1;

  new->actv = 1;
  new->next = NULL;

  CURR->next = new;
  CURR = new;

  return new->fd;
}

int 
delete_fdreg(int fd) 
{
  int deleted = 0;
  struct fdreg * temp = HEAD;

  while(temp->next != NULL) {

    if(temp->next->fd == fd) {

      /* close this fd and make it reusable as the next token */
      close(temp->next->fd);

      free(temp->next);
      temp->next = temp->next->next;

      deleted = 1;
      break;

    }
    
    temp = temp->next;
  }

  /* IMP: recalibrate the 'CURR' */
  temp = HEAD;
  while(temp != NULL) {
    CURR = temp;
    temp = temp->next;
  }

  return deleted;
}

int
fd_one_cli_done(int fd)
{
  struct fdreg * temp = HEAD->next;

  while(temp != NULL) {
  
    if(temp->fd == fd) {

      if(temp->actv > 1) {

        /* one less client to serve this file to */
        temp->actv -= 1;
        return 1;

      } else {

        /* all clients who wanted this file have got it */
        return delete_fdreg(fd);

      }      

      return 1;
    }

    temp = temp->next;
  }

  return 0;
}


void 
print_fd_status()
{
  if(tlist_init == 0) {
    fprintf(stdout, "oops: print_fd_status() was called without initialising token list\n");
    return;
  }

  fprintf(stdout, "------------printing token list------------\n");

  struct fdreg * temp = HEAD->next;

  while(temp != NULL) {
    fprintf(stdout, "\n---fdreg entry---\n");

    fprintf(stdout, "Assigned FD (+token number): %d\n", temp->fd);
    fprintf(stdout, "Active Clients:              %d\n", temp->actv);

    fprintf(stdout, "-----------\n");

    temp = temp->next;
  }
}
