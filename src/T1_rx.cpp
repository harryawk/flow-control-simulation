/*
 * File : T1_rx.cpp
 */
#include "dcomm.h"
#include <stdlib.h>
#include <stddef.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <stdio.h>
// #include <stderr.h>
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 8

Byte rxbuf[RXQSIZE];
char clientName[1000];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false, send_xoff = false;

/* Socket */
int sockfd; //listen on sock_fd

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *);


struct sockaddr_in serv_addr;
socklen_t addrlen = sizeof(serv_addr);

// SERVER PROGRAM
int main(int argc, char *argv[]){
	Byte c;
	/*
	 * Insert code here to bind socket to the port number given in argv[1]
	 */	
	// create socket
	int serversock = socket(AF_INET, SOCK_DGRAM, 0);

	// initialize server address
	

   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
   	if (argc > 1) {
   		serv_addr.sin_port = htons(atoi(argv[2]));
   	} else {
   		serv_addr.sin_port = htons(13514);
   	}
   	//bzero((char *) &serv_addr, sizeof(serv_addr));

   	if (bind(serversock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
   		perror("ERROR : on binding");
   		exit(1);
   	} 
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &serv_addr.sin_addr, clientName, sizeof (clientName));
	printf("Binding pada %s:%d\n", clientName, ntohs(serv_addr.sin_port));
	
	/* Initialize XON/XOFF flags */
	send_xon = false;
	send_xoff = false;
	
	/////////// GET HERE /////////////////////

	/* Create child process */
	pid_t pid = fork();
	/*** IF PARENT PROCESS ***/
	if (pid > 0) {
		while(true){
			c = *(rcvchar(sockfd, rxq));	
			/* Quit on end of file */
			if (c == Endfile){
				exit(0);
			}
		}
	} 
	/*** ELSE IF CHILD PROCESS ***/
	else if (pid == 0) {
		while(true){
			
			break;
	 		/* Call q_get */
	 		/* Can introduce some delay here. */
	 	}
	}
	return 0;
}

static Byte *rcvchar(int sockfd, QTYPE *queue){
	
	//gayakin sama ini plis
	char c[10];
	recvfrom(sockfd, c, 1, 0, (struct sockaddr*)&serv_addr , &addrlen);
	queue->count++;
	queue->data[queue->rear] = c[0];
	queue->rear++;
	/*
	 * Insert code here.
	 * Read a character from socket and put it to the receive buffer.
	 * If the number of characters in the receive buffer is above
	 * certain level, then send XOFF and set a flag (why?).
	 * Return a pointer to the buffer where data is put.
	 */
	return queue->data; 
}

/* q_get returns a pointer to the buffer where data is read 
 * or NULL if buffer is empty/
 */
static Byte *q_get(QTYPE *queue){
	Byte *current;
	/* Nothing in the queue */
	if(!queue->count) return (NULL);
		
	/*
	 * Insert code here.
	 * Retrieve data from buffer, save it to "current" and "data"
	 * If the number of characters in the recieve buffer is below
	 * certain level, then send XON.
	 * Increment front index and check for wraparound.
	 */
}
