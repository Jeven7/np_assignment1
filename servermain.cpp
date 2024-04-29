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
#include <pthread.h> // Mutiple thread
#include <queue>     // used to manage the service
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass argument during compilation '-DDEBUG'
#define DEBUG

#define BACKLOG 5
#define MAX_CLIENTS 5 // Max client number
int client_count = 0;
pthread_mutex_t client_change = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t service = PTHREAD_MUTEX_INITIALIZER;
char* msg = (char*)malloc(1450);
// char* client_msg = (char*)malloc(1450);
char client_msg[2000];
using namespace std;


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int recieveMessage(int &socket_desc, char* client_message, unsigned int msg_size){

  //Clears the message array before recieving the server msg.
  //Because for some reason the function doesnt do it by itself
  memset(client_message, 0, msg_size);
  if(recv(socket_desc, client_message, msg_size, 0) < 0){
  	#ifdef DEBUG
  	printf("Error receiving message\n");
  	#endif
  	return -1;
  }
  else 
    printf("Receive message from client: %s\n",client_message);
  return 0;
} 

int sendMessage(int &socket_desc, char* message, unsigned int msg_size){

  //Sends message to client, clearing the array at the end to not interfere with
  //the messages we put in before calling the function
  if(send(socket_desc, message, msg_size, 0) < 0){
    #ifdef DEBUG
      printf("Unable to send message\n");
  	#endif
  	return -1;
  }
  else 
    printf("Send to client: %s\n",message);
  return 0;
}

void *handle_client(void *arg) {
  int accept_sockfd = *(int *)arg;

  double fv1, fv2, fresult, client_fresult, ans;
  int iv1, iv2, iresult, client_iresult;
  // char msg[1450];
  char *op = randomType();

  struct sockaddr_storage connector_address; // Connectors address info
  socklen_t sin_size = sizeof(connector_address);
  char s[INET6_ADDRSTRLEN]; // Array for storing IP address
  if (client_count >= MAX_CLIENTS) {
      // send busy message to the client
      /* clear string for sending */
      printf("One client is rejected\n\n");
      sprintf(msg, "Server wait queue is full. Please try again later.\n");
      send(accept_sockfd, msg, strlen(msg),0);
      close(accept_sockfd);
      pthread_exit(NULL);
      
  }
  pthread_mutex_lock(&client_change);
  client_count++;
  pthread_mutex_unlock(&client_change);
  // Get client address info
  if (getpeername(accept_sockfd, (struct sockaddr *)&connector_address, &sin_size) == -1) {
      perror("getpeername");
      close(accept_sockfd);
      pthread_mutex_lock(&client_change);
      client_count--;
      pthread_mutex_unlock(&client_change);
      pthread_exit(NULL);
  }

  // //Converts IPv4 or IPv6 Internet network addresses into strings in the Internet Standard format
  inet_ntop(connector_address.ss_family, get_in_addr((struct sockaddr *)&connector_address), s, sizeof(s));

  #ifdef DEBUG
      printf("server: Connection from %s:%d\n\n", s, ntohs(((struct sockaddr_in *)&connector_address)->sin_port));
  #endif

  struct timeval timeout;
  timeout.tv_sec = 5;//Set the timeout to be 5 seconds
  timeout.tv_usec = 0;
  setsockopt(accept_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
  if(accept_sockfd == -1){
    perror("accept");
    pthread_mutex_lock(&client_change);
    client_count--;
    pthread_mutex_unlock(&client_change);
    pthread_exit(NULL);
  }
  pthread_mutex_lock(&service);
  // Send initial message to client
  memset(msg, sizeof(msg),0);
  sprintf(msg, "TEXT TCP 1.0\n\n");
  if ((sendMessage(accept_sockfd, msg, strlen(msg))) == -1) {
      perror("send");
      close(accept_sockfd);
      pthread_mutex_lock(&client_change);
      client_count--;
      pthread_mutex_unlock(&client_change); 
      pthread_mutex_unlock(&service);
      pthread_exit(NULL);
  }
  // Receive response from client
  if ((recieveMessage(accept_sockfd, client_msg, sizeof(client_msg))) == -1) {
      // Handle timeout or error
      memset(msg, sizeof(msg),0);
      sprintf(msg, "ERROR TO\n");
      #ifdef DEBUG
      printf("No answer from client... \n");
      #endif
      sendMessage(accept_sockfd, msg, strlen(msg));
      close(accept_sockfd);
      pthread_mutex_lock(&client_change);
      client_count--;
      pthread_mutex_unlock(&client_change);
      pthread_mutex_unlock(&service);
      pthread_exit(NULL);
  }
  // sleep(30);
  if (strcmp(client_msg, "OK\n") == 0) {
    if (initCalcLib() != 0) {
        perror("calclib init");
    }

    // Perform calculations based on operation type
    if (op[0] == 'f') { // Randomize float numbers
      fv1 = randomFloat();
      fv2 = randomFloat();
      #ifdef DEBUG
      printf("\nExpected calculation: %s %8.8g %8.8g\n", op,fv1,fv2);
      #endif
      memset(msg, sizeof(msg),0);
      sprintf(msg, "%s %8.8g %8.8g\n", op, fv1, fv2);
      // Send operation and operands to client
      if ((sendMessage(accept_sockfd, msg, strlen(msg))) == -1) {
          perror("send");
          close(accept_sockfd);
          pthread_mutex_lock(&client_change);
          client_count--;
          pthread_mutex_unlock(&client_change);
          pthread_mutex_unlock(&service);
          pthread_exit(NULL);
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
      // Receive client's calculation result
      if ((recieveMessage(accept_sockfd, client_msg, sizeof(client_msg))) == -1) {
          // Handle timeout or error
          memset(msg, sizeof(msg),0);
          sprintf(msg, "ERROR TO\n");
          #ifdef DEBUG
            printf("No answer from client... \n\n");
          #endif
          sendMessage(accept_sockfd, msg, strlen(msg));
          pthread_mutex_lock(&client_change);
          client_count--;
          pthread_mutex_unlock(&client_change);
          close(accept_sockfd);
          pthread_mutex_unlock(&service);
          pthread_exit(NULL);
      }
      sscanf(client_msg, "%lg", &client_fresult);
      #ifdef DEBUG
        printf("Got answer %8.8g Expected answer: %8.8g \n", client_fresult, fresult);
      #endif
      // Compare client's result with server's result
      ans = abs(client_fresult - fresult);
      // Send appropriate response to client
      if (ans < 0.0001) {
        memset(msg, sizeof(msg),0);
        sprintf(msg, "OK\n");
        #ifdef DEBUG
          printf("The answer is right. Sent OK.\n\n");
        #endif
      } 
      else {
        memset(msg, sizeof(msg),0);
        sprintf(msg, "ERROR\n");
        #ifdef DEBUG
          printf("The answer is wrong. Sent ERROR.\n\n");
        #endif
      }
      if ((sendMessage(accept_sockfd, msg, strlen(msg))) == -1) {
          perror("send");
          close(accept_sockfd);
          pthread_mutex_lock(&client_change);
          client_count--;
          pthread_mutex_unlock(&client_change);
          pthread_mutex_unlock(&service);
          pthread_exit(NULL);
      }
    } 
    else { // Randomize int numbers
      iv1 = randomInt();
      iv2 = randomInt();
      #ifdef DEBUG
        printf("\nExpected calculation: %s %d %d \n", op, iv1, iv2);
      #endif
      sprintf(msg, "%s %d %d \n", op, iv1, iv2);
      if((sendMessage(accept_sockfd, msg, strlen(msg))) == -1){
        perror("send");
        close(accept_sockfd);
        pthread_mutex_lock(&client_change);
        client_count--;
        pthread_mutex_unlock(&client_change);
        pthread_mutex_unlock(&service);
        pthread_exit(NULL);
      }
      if(strcmp(op,"add")==0){
        iresult=iv1+iv2;
      }
      else if (strcmp(op, "sub")==0){
        iresult=iv1-iv2;
      }
      else if (strcmp(op, "mul")==0){
        iresult=iv1*iv2;
      }
      else if (strcmp(op, "div")==0){
        iresult=iv1/iv2;
      }
      if((recieveMessage(accept_sockfd, client_msg, sizeof(client_msg))) == -1){
        memset(msg, sizeof(msg),0);
        sprintf(msg, "ERROR TO\n");
        #ifdef DEBUG
          printf("No answer from client... \n\n");
        #endif
        send(accept_sockfd, &msg, strlen(msg), 0);
        close(accept_sockfd);
        pthread_mutex_lock(&client_change);
        client_count--;
        pthread_mutex_unlock(&client_change);
        pthread_mutex_unlock(&service);
        pthread_exit(NULL);
      }
      sscanf(client_msg, "%d", &client_iresult);
      
      ans = abs(client_iresult-iresult);
      #ifdef DEBUG
        printf("Got anser %d Expected answer: %d \n", client_iresult, iresult);
      #endif
      if(ans < 0.0001){
        memset(msg, sizeof(msg),0);
        sprintf(msg, "OK\n");
        #ifdef DEBUG
          printf("The answer is right. Sent OK\n\n");
        #endif
      }
      else{
        memset(msg, sizeof(msg),0);
        sprintf(msg, "ERROR\n");
        #ifdef DEBUG
          printf("The answer is wrong. Sent ERROR\n\n");
        #endif        
      }
      if ((sendMessage(accept_sockfd, msg, strlen(msg))) == -1) {
        perror("send");
        close(accept_sockfd);
        pthread_mutex_lock(&client_change);
        client_count--;
        pthread_mutex_unlock(&client_change);
        pthread_mutex_unlock(&service);
        pthread_exit(NULL);
      }
    }
    pthread_mutex_lock(&client_change);
    client_count--;
    pthread_mutex_unlock(&client_change);
    close(accept_sockfd);
    pthread_mutex_unlock(&service);
    pthread_exit(NULL);
  }
}


int main(int argc, char *argv[]){
  char *Desthost;
  char *Destport;
  if(argc != 2){
    fprintf(stderr, "Please enter the correct form: <ip>:<port> \n");
    exit(1);
  }
  else{
    // char delim[]=":";
    // Desthost=strtok(argv[1],delim);
    // Destport=strtok(NULL,delim);
    char *lastColon = strrchr(argv[1], ':'); 
    if(lastColon == NULL) {
        fprintf(stderr, "Invalid format. Please enter the correct form: <ip>:<port> \n");
        exit(1);
    }

    *lastColon = '\0'; 
    Desthost = argv[1];
    Destport = lastColon + 1; 
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
    #ifdef DEBUG  
      printf("Host %s, and port %d.\n",Desthost,port);
    #endif
    void *addr;
    if (p->ai_family == AF_INET) { // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    inet_ntop(p->ai_family, addr, Desthost, sizeof(Desthost));
    break;
  }
  freeaddrinfo(servinfo);
  
  if(p == NULL) {
    fprintf(stderr, "Server failed to bind \n");
    exit(1);
  }
  
  if((listen(sockfd, BACKLOG)) == -1){
    perror("listen");
    exit(1);
  }
  // sleep(60);
  while (1) {
    struct sockaddr_storage connector_address;
    socklen_t sin_size = sizeof connector_address;
    int accept_sockfd;

    // accept connection
    accept_sockfd = accept(sockfd, (struct sockaddr *)&connector_address, &sin_size);
    if (accept_sockfd == -1) {
        perror("accept");
        continue;
    }

    // create new client thread to deal with the service
    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, &accept_sockfd) != 0) {
        perror("pthread_create");
        close(accept_sockfd);
        continue;
    }
    
    pthread_detach(tid);
  }
  return 0;
}

