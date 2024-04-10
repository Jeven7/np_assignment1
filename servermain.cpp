/*
 * @Author: Yiwen Jiang
 * @Date: 2024-04-06
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass argument during compilation '-DDEBUG'
#define DEBUG

#define BACKLOG 5


using namespace std;

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[]){
  char *Desthost;
  char *Destport;
  if(argc != 2){
    // fprintf(stderr, "Usage; <ip>:<port> \n");
    // exit(1);
    Desthost="127.0.0.1";
    Destport="5000";
  }
  else{
    char delim[]=":";
    Desthost=strtok(argv[1],delim);
    Destport=strtok(NULL,delim);
  }
  
  // If we have the input host, *Desthost points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 
  //otherwise, *Desthost points to "127.0.0.1"
  // *Dstport points to "5000" 

    /* Do magic */
  int port=atoi(Destport);

  int sockfd, new_sockfd;//Listening o sockfd, new connection on new_sockfd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_address; //Connectors address info
  socklen_t sin_size = sizeof(their_address);
  int yes = 1;
  char s[INET6_ADDRSTRLEN]; //Should the server be able to handle IPV6 address?
  int rv;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; //Should the server be able to handle IPV6 address?
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if((rv = getaddrinfo(NULL, Destport, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo : %s \n", gai_strerror(rv));
    return 1;//Should the server be able to handle IPV6 address?
  }

  for(p = servinfo; p != NULL; p = p->ai_next){
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("Server: socket");
      continue;
    }
    if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1){
      perror("setsockopt ");
      exit(1);
    }
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      perror("Server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);
  #ifdef DEBUG  
  printf("Host %s, and port %d.\n",Desthost,port);
  #endif
  if(p == NULL) {
    fprintf(stderr, "Server failed to bind \n");
    exit(1);
  }
  
  if((listen(sockfd, BACKLOG)) == -1){
    perror("listen");
    exit(1);
  }
 
  char msg[1500];
  char command[10];
  int i1;
  int i2;
  int iresult, cIResult;
  double f1;
  double f2;
  double fresult, cFResult, ans;


  int childCount = 0;
  int readsize;
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  while(1){
    new_sockfd = accept(sockfd, (struct sockaddr*)&their_address, &sin_size);
    setsockopt(new_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    if(new_sockfd == -1){
      perror("accept");
      continue;
    }
    inet_ntop(their_address.ss_family, get_in_addr((struct sockaddr *)&their_address), s, sizeof(s));
    #ifdef DEBUG
      printf("server: Connection %d from %s\n",childCount, s);
    #endif
    strcpy(msg, "TEXT TCP 1.0\n\n");
    if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
      perror("send");
      close(new_sockfd);
      continue;
    }
    //while(1){
      msg[readsize - 1] = '\0';
      if((readsize = recv(new_sockfd, &msg, strlen(msg), 0)) == -1){
        strcpy(msg, "ERROR TO\n");
        #ifdef DEBUG
        printf("No answer from client... \n");
        #endif
        readsize = send(new_sockfd, msg, strlen(msg), 0);
        close(new_sockfd);
        continue;
      }
      msg[readsize - 1] = '\0';
      #ifdef DEBUG
      printf("server: Recieved a message %s \n", msg);
      #endif

      if(strcmp(msg, "OK") == 0){
        initCalcLib();
        sprintf(command, "%s", randomType());
        if(command[0] == 'f'){ //radomize float numbers
          f1 = randomFloat();
          f2 = randomFloat();
          #ifdef DEBUG
            printf("\n%s \n", command);
          #endif
          sprintf(msg, "%s %8.8g %8.8g \n", command, f1, f2);
          if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(new_sockfd);
            continue;
          }
          if(strcmp(command,"fadd")==0){
      	  	fresult=f1+f2;
    	  	} else if (strcmp(command, "fsub")==0){
      		  fresult=f1-f2;
    		  } else if (strcmp(command, "fmul")==0){
       	  	fresult=f1*f2;
    	  	} else if (strcmp(command, "fdiv")==0){
      		  fresult=f1/f2;
    	    }
          if((readsize = recv(new_sockfd, &msg, sizeof(msg), 0)) == -1){
            strcpy(msg, "ERROR TO\n");
            #ifdef DEBUG
             printf("No answer from client... \n");
             #endif
            readsize = send(new_sockfd, msg, strlen(msg), 0);
            close(new_sockfd);
            continue;
         }
         msg[readsize - 1] = '\0';
         rv = sscanf(msg, "%lg", &cFResult);
         #ifdef DEBUG
             printf("Got answer %8.8g My answer: %8.8g \n", cFResult, fresult);
          #endif
          ans = abs(cIResult-iresult);
         if(ans < 0.0001){
           strcpy(msg, "OK\n");
           #ifdef DEBUG
             printf("Sent OK\n");
            #endif
           if((readsize = send(new_sockfd, msg, strlen(msg), 0) == -1)){
            perror("send");
            close(new_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(new_sockfd);
          continue;
         }else{
           strcpy(msg, "ERROR\n");
           if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(new_sockfd);
            continue;
          }
         }
        }else{
          i1 = randomInt();
          i2 = randomInt();
          #ifdef DEBUG
            printf("%s %d %d ", command, i1, i2);
          #endif
          sprintf(msg, "%s %d %d \n", command, i1, i2);
          if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(new_sockfd);
            continue;
          }
          if(strcmp(command,"add")==0){
      		iresult=i1+i2;
    		  } else if (strcmp(command, "sub")==0){
     		    iresult=i1-i2;
     	    	printf("[%s %d %d = %d ]\n",command,i1,i2,iresult);
    		  } else if (strcmp(command, "mul")==0){
      		  iresult=i1*i2;
    	  	} else if (strcmp(command, "div")==0){
        		iresult=i1/i2;
          }
          if((readsize = recv(new_sockfd, &msg, sizeof(msg), 0)) == -1){
            strcpy(msg, "ERROR TO\n");
            #ifdef DEBUG
             printf("No answer from client... \n");
             #endif
            readsize = send(new_sockfd, &msg, strlen(msg), 0);
            close(new_sockfd);
            continue;
         }
         msg[readsize - 1] = '\0';
         rv = sscanf(msg, "%d", &cIResult);
         
          ans = abs(cIResult-iresult);
          #ifdef DEBUG
             printf("Got anser %d My answer: %d \n", cIResult, iresult);
          #endif
         if(ans < 0.0001){
           strcpy(msg, "OK\n");
           #ifdef DEBUG
             printf("Sent OK\n");
            #endif
           if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(new_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(new_sockfd);
          continue;
         }else{
           strcpy(msg, "ERROR\n");
           if((readsize = send(new_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(new_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
         }

        }
      }else{
        close(new_sockfd);
        continue;
      }
      

  

  return 0;
  }
}
