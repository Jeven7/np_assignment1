#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define SA struct sockaddr
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG


// Included to get the support library
#include <calcLib.h>
//Function to recieve a message from the server
void recieveMessage(int &socket_desc, char* server_message, unsigned int msg_size){

  //Clears the message array before recieving the server msg.
  //Because for some reason the function doesnt do it by itself
  memset(server_message, 0, msg_size);
  if(recv(socket_desc, server_message, msg_size, 0) < 0){
  	#ifdef DEBUG
  	printf("Error receiving message\n");
  	#endif
  	exit(-1);
  }
  else 
    printf(server_message);
} 
void sendMessage(int &socket_desc, char* client_message, unsigned int msg_size){

  //Sends message to client, clearing the array at the end to not interfere with
  //the messages we put in before calling the function
  if(send(socket_desc, client_message, msg_size, 0) < 0){
	#ifdef DEBUG
  	printf("Unable to send message\n");
  	#endif DEBUG
  	exit(-1);
  }
  else printf(client_message);
 
}
void calculateMessage(char* server_message, int &socket_desc){
	int i1, i2, iresult;
	float f1, f2, fresult;
	char* operation, *grab1, *grab2, *temp1, *temp2;
	
	if(server_message[0] == 'f'){
		operation = strtok(server_message, " ");
		grab1 = strtok(NULL, " ");
		grab2 = strtok(NULL, "\\");
		f1 = atof(grab1); 
		f2 = atof(grab2);
		
		if(strcmp(operation, "fadd") == 0){
			fresult = f1+f2;
		} else if(strcmp(operation, "fsub") == 0){
			fresult = f1-f2;
		} else if(strcmp(operation, "fmul") == 0){
			fresult = f1*f2;
		} else if(strcmp(operation, "fdiv") == 0){
			fresult = f1/f2;
		}
		
		char* str = (char*)malloc(1450);
		sprintf(str, "%8.8g\n", fresult);
		//TODO UNCOMMENT THIS
		sendMessage(socket_desc, str, strlen(str));
		return;
		
	} else {
	
	  operation = strtok(server_message, " ");
		grab1 = strtok(NULL, " ");
		grab2 = strtok(NULL, "\\");
		i1 = atoi(grab1); 
		i2 = atoi(grab2);
		
		if(strcmp(operation, "add") == 0){
			iresult = i1+i2;
		} else if(strcmp(operation, "sub") == 0){
			iresult = i1-i2;
		} else if(strcmp(operation, "mul") == 0){
			iresult = i1*i2;
		} else if(strcmp(operation, "div") == 0){
		//Make result into string then add \n
			iresult = i1/i2;
		}
		char* strResult = (char*)malloc(1450);
		sprintf(strResult, "%d\n", iresult);
		//TODO UNCOMMENT THIS
		sendMessage(socket_desc, strResult, strlen(strResult));
		return;
	}


}

int CAP = 2000;
int main(int argc, char *argv[]){


	//Variables
	char* splits[CAP];
  //char* p = strtok(argv[1], ":");
  int delimCounter = 0;
  char *Desthost;
  char *Destport;
	int serverfd;
	struct sockaddr_in client;
	char server_message[CAP];
  //Get argv
  if (argc != 2) {
		fprintf(stderr, "Please enter the correct form: <ip>:<port> \n");
		exit(1);
	}
	else {
		// char delim[]=":";
		// Desthost=strtok(argv[1],delim);
		// Destport=strtok(NULL,delim);
		char* lastColon = strrchr(argv[1], ':');
		if (lastColon == NULL) {
			fprintf(stderr, "Invalid format. Please enter the correct form: <ip>:<port> \n");
			exit(1);
		}

		*lastColon = '\0';
		Desthost = argv[1];
		Destport = lastColon + 1;
	}
	int port=atoi(Destport);
	printf("Host %s, and port %d.\n", Desthost, port);
	
	int sockfd, new_fd;//Listening o sockfd, new connection on accept_sockfd
	struct addrinfo hints, * servinfo, * p;
	struct sockaddr_storage connector_address; //Connectors address info
	socklen_t sin_size = sizeof(connector_address);
	struct sockaddr_in server_addr;
	int choice = 1;// Integer for setting socket option
	char s[INET6_ADDRSTRLEN];// Array for storing IP address
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //supports both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM;//Socket type is TCP
	hints.ai_flags = AI_PASSIVE;// Use passive mode for server

	if ((rv = getaddrinfo(NULL, Destport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo : %s \n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		//Get the address of the server, create a listening socket, and bind it to the specified address
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			//Create a socket based on the current address information struct, if the creation fails, print the error message and move on to the next struct
			perror("Server: socket");
			continue;
		}

		void* addr;
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
		}
		else { // IPv6
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}
		inet_ntop(p->ai_family, addr, Desthost, sizeof(Desthost));
		break;
	}
#ifdef DEBUG  
	printf("Host %s, and port %d.\n", Desthost, port);
#endif

  //Create Socket Structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(Desthost);
  int error;
  
  //Establish Connection
  error = connect(sockfd, p->ai_addr, p->ai_addrlen);
  if(error < 0){
  	#ifdef DEBUG
  	printf("Unable to connect\n");
  	printf("Error: %d \n", errno);
  	#endif
  	return -1;
  }
  #ifdef DEBUG
  else printf("Connected\n");
  #endif
  
  //Recieve message from server
  recieveMessage(sockfd, server_message, sizeof(server_message));
  
  //Compare strings to verify version

  if(strcmp(server_message,"TEXT TCP 1.0\n\n") == 0){
  	#ifdef DEBUG
  	printf("Same\n");
  	#endif
  	char* str = "OK\n";
  	//strcpy(client_message, str);
  	//Send back the OK
  	sendMessage(sockfd, str, strlen(str));
  }
  else{
	  printf("Closing connection\n");
	  close(sockfd);
	  return -1;
  }

	// sleep(4);
  //Recieve the problem
  printf("We are here\n");
  recieveMessage(sockfd, server_message, sizeof(server_message));
	// sleep(6);
  if(strcmp(server_message, "ERROR TO\n") == 0){
	  printf("We got TO'ed. Closing connection\n");
	  close(sockfd);
	  return -1;
  }
  //Translate Message
  calculateMessage(server_message, sockfd);
  
  //Send answer to server
  //sendMessage(socket_desc, client_message, sizeof(client_message));
  
  //Recieve the final Message
  recieveMessage(sockfd, server_message, sizeof(server_message));

  if(strcmp(server_message, "ERROR TO\n") == 0){
	  printf("We got TO'ed. Closing connection\n");
	  close(sockfd);
	  return -1;
  }
  //Close socket and quit program
  //TODO
  close(sockfd);
  return 0;
}
