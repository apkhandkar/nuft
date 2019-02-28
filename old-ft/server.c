#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct cmsg {
  int cm_type;
  int cm_cblk;
  int cl_token;
  char cm_body[256];
};

struct smsg {
  int sm_type;
  int sm_nblk;
  char sm_body[1024];
};

/*
because of how tokens are assigned to each client, this server can service 
9999 clients in its lifetime; it automatically shuts off after that
*/
int desc_array[10000];
int nblk_array[10000];
int descarr_top = 0;

int main(int argc, char ** argv)
{
  int sockfd, len, n;
  int lblk_sz;
  int lblk_req = 0;
  struct sockaddr_in servaddr, cliaddr;
  struct stat st;

  if(argc != 2) {
    printf("usage: server <port>\n");
    exit(0);
  }

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(-1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(atoi(argv[1]));


  if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0) {
    perror("bind");
    exit(-1);
  }

  printf("started server @ port %d\n", atoi(argv[1]));

  struct cmsg * recv_mesg = (struct cmsg*)malloc(sizeof(struct cmsg));
  struct smsg * send_mesg = (struct smsg*)malloc(sizeof(struct smsg));

  while(1) {
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, recv_mesg, sizeof(struct cmsg), 0, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
    
    if(recv_mesg->cm_type == 0) {

      // have we serviced 9999 clients?
      if(descarr_top == 10000) {
        // time to die...
        printf("this server has lived its life... quitting\n");
        for(int i=0; i<10000; i++) {
          // ...not before closing all files, though
          close(desc_array[i]);
          close(sockfd);
        }
        exit(0);
      }

      // client is requesting for a file
      printf("client requested %s\n", recv_mesg->cm_body);

      if((desc_array[descarr_top] = open(recv_mesg->cm_body, O_RDONLY)) < 0) {
        fprintf(stderr, "%s couldn't be opened; bad filename/file not found\n", recv_mesg->cm_body);
        send_mesg->sm_type = -1;
      } else {
        if(fstat(desc_array[(descarr_top)], &st) < 0) {
          fprintf(stderr, "couldn't get size of %s\n", recv_mesg->cm_body);
          send_mesg->sm_type = -1;
        } else {
          // update descriptor array
          descarr_top += 1;
          printf("token to client: %d fd: %d\n", (descarr_top-1), desc_array[(descarr_top-1)]);
          // calculate number of blocks file will be transferred in
          nblk_array[(descarr_top-1)] = (int)ceil((long double)st.st_size/1024);

          // calculate size of last block
          lblk_sz = (int)(((int)st.st_size) - 1024*(nblk_array[(descarr_top-1)]-1));

          // transmit number of blocks and size of last block to client
          send_mesg->sm_type = 0;
          send_mesg->sm_nblk = nblk_array[(descarr_top-1)]; 
          sprintf(send_mesg->sm_body, "%d %d", lblk_sz, (descarr_top-1));
        }
      }
      // send number of blocks client should expect
      sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);
      printf("sending %s in %d blocks\n", recv_mesg->cm_body, nblk_array[(descarr_top-1)]);

    } else if(recv_mesg->cm_type == 1) {
      if(recv_mesg->cm_cblk == (nblk_array[recv_mesg->cl_token] - 1)) {
       
        lblk_req = 1; 

      } 
      if((lseek(desc_array[recv_mesg->cl_token], 0, SEEK_CUR)/1024) != recv_mesg->cm_cblk) {
      
        // block to be read by server and block client is requesting do not match
        // this might be because the client wants to resume an interrupted download
        // jump to the requested block
        lseek(desc_array[recv_mesg->cl_token], (recv_mesg->cm_cblk*1024), SEEK_SET); 
          
      } 

      send_mesg->sm_type = 1;
      send_mesg->sm_nblk = recv_mesg->cm_cblk;
      read(desc_array[recv_mesg->cl_token], send_mesg->sm_body, 1024);
      sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);
  
      if(lblk_req) {
        printf("finished a transfer...\n");
        close(desc_array[recv_mesg->cl_token]);
        lblk_req = 0;
      }

    } else if(recv_mesg->cm_type == -1) {

      printf("client encountered error, cancelling transfer\n");
      close(desc_array[recv_mesg->cl_token]);
    } else {
      printf("client: bad request format\n");
    }
  }

  return 0;
}
