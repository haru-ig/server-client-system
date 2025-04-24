//chatclient.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

void errorHandler(int sock) {
  close(sock);
  exit(1);
}

int main(int argc, char **argv) {
  int sock;
  struct sockaddr_in svraddr;
  struct hostent *svr;
  char *hostname;
  char *username;
  char rbuf[1024];
  int nbytes;
  fd_set rfds;       
  struct timeval tv;

  hostname = argv[1];
  username = argv[2];

  /* c1 */

     /*create the socket*/
  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }
  
/*get the server's DNS entry*/
   svr = gethostbyname(hostname);
   if(svr == NULL) {
       fprintf(stderr, "Error, no such host as %s\n", hostname);
       exit(0);
   }

   /*build the server's Internet address*/
   bzero((char *) &svraddr, sizeof(svraddr));
   svraddr.sin_family = AF_INET;
   bcopy((char *)svr->h_addr, (char *) &svraddr.sin_addr, svr->h_length);
   svraddr.sin_port = htons(10140);



    /*create a connection with the server*/
   if(connect(sock, (struct sockaddr *)&svraddr, sizeof(svraddr)) < 0) {
       perror("Error connecting");
       exit(1);
   }


  printf("connected to %s\n", argv[1]);

  /* c2 */
  nbytes = read(sock, rbuf, 17);
  
  if (nbytes < 17 || strncmp(rbuf, "REQUEST ACCEPTED\n", 17) != 0) {
    fprintf(stderr, "REQUEST REJECTED\n");
    errorHandler(sock);
  }

  printf("REQUEST ACCEPTED\n");

  /* c3 */
  nbytes = snprintf(rbuf, sizeof(rbuf), "%s\n", username);
  write(sock, rbuf, nbytes);
  nbytes = read(sock, rbuf, 20);
 
  if (nbytes < 20 || strncmp(rbuf, "USERNAME REGISTERED\n", 20) != 0) {
    write(0, rbuf, nbytes);
    errorHandler(sock);
  }

  printf("USERNAME REGISTERED\n");

  do
  {
    /* c4 */

    
    FD_ZERO(&rfds);      
    FD_SET(0, &rfds);    
    FD_SET(sock, &rfds); 

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (select(sock + 1, &rfds, NULL, NULL, &tv) > 0) {
      if (FD_ISSET(0, &rfds)) { 
        if ((nbytes = read(0, rbuf, sizeof(rbuf))) < 0) {
          perror("Error reading from stdin");
        }
        else if(nbytes == 0) {
          break;
        }

        write(sock, rbuf, nbytes);
      }
      if (FD_ISSET(sock, &rfds)) { 
        if ((nbytes = read(sock, rbuf, sizeof(rbuf))) < 0) {
          perror("Error reading from socket");
        } else if (nbytes == 0) {
          break;
        }
       
        write(1, rbuf, nbytes);
      }
    }
  } while (1); 
  /* c5 */
  close(sock);
  exit(0);

  return 0;
}

