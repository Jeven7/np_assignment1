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
    fprintf(stderr, "Please enter the correct form: <ip>:<port> \n");
    exit(1);
    // Desthost="127.0.0.1";
    // Destport="5000";
  }
  else{
    char delim[]=":";
    Desthost=strtok(argv[1],delim);
    Destport=strtok(NULL,delim);
  }
  
  // If we have the input host, *Desthost points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 
  //otherwise, ask the user to input the correct form of address

    /* Do magic */
  int port=atoi(Destport);

  int sockfd, accept_sockfd;//Listening o sockfd, new connection on accept_sockfd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage connector_address; //Connectors address info
  socklen_t sin_size = sizeof(connector_address);
  int choice = 1;// Integer for setting socket option
  char s[INET6_ADDRSTRLEN];// Array for storing IP address
  int rv;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; //supports both IPv4 and IPv6
  hints.ai_socktype = SOCK_STREAM;//Socket type is TCP
  hints.ai_flags = AI_PASSIVE;// Use passive mode for server

  if((rv = getaddrinfo(NULL, Destport, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo : %s \n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next){
    //Get the address of the server, create a listening socket, and bind it to the specified address
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      //Create a socket based on the current address information struct, if the creation fails, print the error message and move on to the next struct
      perror("Server: socket");
      continue;
    }
    if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &choice, sizeof(int))) == -1){
      //Set the socket option to allow rebinding of addresses. If the setup fails, an error message is printed and the program is exited.
      perror("setsockopt ");
      exit(1);
    }
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      //Bind the socket to the specified address
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
  // sleep(30);
  double fv1,fv2,fresult,client_fresult,ans;
  int iv1,iv2,iresult,client_iresult;
  char msg[1450];
  char *op=randomType();
  int readsize;
  struct timeval timeout;
  timeout.tv_sec = 5;//Set the timeout to be 5 seconds
  timeout.tv_usec = 0;
  while(1){
    accept_sockfd = accept(sockfd, (struct sockaddr*)&connector_address, &sin_size);
    setsockopt(accept_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    if(accept_sockfd == -1){
      perror("accept");
      continue;
    }
    //Converts IPv4 or IPv6 Internet network addresses into strings in the Internet Standard format
    inet_ntop(connector_address.ss_family, get_in_addr((struct sockaddr *)&connector_address), s, sizeof(s));
    #ifdef DEBUG
      printf("server: Connection client\n");
    #endif
    strcpy(msg, "TEXT TCP 1.0\n\n");
    if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
      perror("send");
      close(accept_sockfd);
      continue;
    }
    //while(1){
      msg[readsize - 1] = '\0';
      if((readsize = recv(accept_sockfd, &msg, strlen(msg), 0)) == -1){
        //timeout
        strcpy(msg, "ERROR TO\n");
        #ifdef DEBUG
        printf("No answer from client... \n");
        #endif
        readsize = send(accept_sockfd, msg, strlen(msg), 0);
        close(accept_sockfd);
        continue;
      }
      msg[readsize - 1] = '\0';
      #ifdef DEBUG
      printf("server: Recieved a message %s from client\n", msg);
      #endif

      if(strcmp(msg, "OK") == 0){
        if(initCalcLib()!=0){
            perror("calclib init");
        }
        /* clear string for sending */
        // memset(msg, sizeof(msg),0);
        if(op[0] == 'f'){ //radomize float numbers
          fv1 = randomFloat();
          fv2 = randomFloat();
          #ifdef DEBUG
            printf("\n%s %8.8g %8.8g\n", op,fv1,fv2);
          #endif
          sprintf(msg, "%s %8.8g %8.8g\n", op, fv1, fv2);
          if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          if(strcmp(op,"fadd")==0){
      	  	fresult=fv1+fv2;
    	  	} 
          else if (strcmp(op, "fsub")==0){
      		  fresult=fv1-fv2;
    		  } 
          else if (strcmp(op, "fmul")==0){
       	  	fresult=fv1*fv2;
    	  	} 
          else if (strcmp(op, "fdiv")==0){
      		  fresult=fv1/fv2;
    	    }
          if((readsize = recv(accept_sockfd, &msg, sizeof(msg), 0)) == -1){
            strcpy(msg, "ERROR TO\n");
            #ifdef DEBUG
             printf("No answer from client... \n\n");
             #endif
            readsize = send(accept_sockfd, msg, strlen(msg), 0);
            close(accept_sockfd);
            continue;
         }
         msg[readsize - 1] = '\0';
         rv = sscanf(msg, "%lg", &client_fresult);
         #ifdef DEBUG
             printf("Got answer %8.8g Expected answer: %8.8g \n", client_fresult, fresult);
          #endif
          ans = abs(client_fresult-fresult);
         if(ans < 0.0001){
           strcpy(msg, "OK\n");
           #ifdef DEBUG
             printf("The answer is right. Sent OK.\n\n");
            #endif
           if((readsize = send(accept_sockfd, msg, strlen(msg), 0) == -1)){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(accept_sockfd);
          continue;
         }
         else{
           strcpy(msg, "ERROR\n");
           #ifdef DEBUG
             printf("The answer is wrong. Sent ERROR.\n\n");
            #endif
           if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(accept_sockfd);
          continue;
         }
        }
        else{//radomize int numbers
          iv1 = randomInt();
          iv2 = randomInt();
          #ifdef DEBUG
            printf("%s %d %d ", op, iv1, iv2);
          #endif
          sprintf(msg, "%s %d %d \n", op, iv1, iv2);
          if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          if(strcmp(op,"add")==0){
      		  iresult=iv1+iv2;
    		  }
          else if (strcmp(op, "sub")==0){
     		    iresult=iv1-iv2;
     	    	// printf("[%s %d %d = %d ]\n",op,iv1,iv2,iresult);
    		  }
          else if (strcmp(op, "mul")==0){
      		  iresult=iv1*iv2;
    	  	}
          else if (strcmp(op, "div")==0){
        		iresult=iv1/iv2;
          }
          if((readsize = recv(accept_sockfd, &msg, sizeof(msg), 0)) == -1){
            strcpy(msg, "ERROR TO\n");
            #ifdef DEBUG
             printf("No answer from client... \n\n");
             #endif
            readsize = send(accept_sockfd, &msg, strlen(msg), 0);
            close(accept_sockfd);
            continue;
         }
         msg[readsize - 1] = '\0';
         rv = sscanf(msg, "%d", &client_iresult);
         
          ans = abs(client_iresult-iresult);
          #ifdef DEBUG
             printf("Got anser %d Expected answer: %d \n", client_iresult, iresult);
          #endif
         if(ans < 0.0001){
           strcpy(msg, "OK\n");
           #ifdef DEBUG
             printf("The answer is right. Sent OK\n\n");
            #endif
           if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(accept_sockfd);
          continue;
         }
         else{
           strcpy(msg, "ERROR\n");
           #ifdef DEBUG
             printf("The answer is wrong. Sent ERROR\n\n");
            #endif
           if((readsize = send(accept_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(accept_sockfd);
            continue;
          }
          msg[readsize - 1] = '\0';
          close(accept_sockfd);
          continue;
         }

        }
      }else{
        close(accept_sockfd);
        continue;
      }
      

  

  return 0;
  }
}
