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
#include <pthread.h>

// #include <stderr.h>
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 16

Byte rxbuf[RXQSIZE];
char clientName[1000];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false, send_xoff = false;

/* Socket */
int sockfd; //listen on sock_fd

/* Functions declaration */
static Byte *rcvchar(int, QTYPE *);
static Byte *q_get(QTYPE *);

struct sockaddr_in serv_addr;
socklen_t addrlen;

void *childProcess(void *threadid){
	int byte_now = 0;
	while(true){
		Byte *now;
		now = q_get(rxq);
		printf("Mengkonsumsi byte ke-%d: '%c'\n", ++byte_now, *now);
		sleep(1);
		/* Call q_get */
		/* Can introduce some delay here. */
		break;
	}
	pthread_exit(NULL);
}

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
   	if (argc > 2) {
   		serv_addr.sin_port = htons(atoi(argv[2]));
   	} else {
   		serv_addr.sin_port = htons(13514);
   	}
   	//bzero((char *) &serv_addr, sizeof(serv_addr));
	addrlen = sizeof(serv_addr);
   	if (bind(serversock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
   		perror("ERROR : on binding");
   		exit(1);
   	}
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &serv_addr.sin_addr, clientName, sizeof (clientName));
	printf("Binding pada %s:%d\n", clientName, ntohs(serv_addr.sin_port));

	/* Initialize XON/XOFF flags */
	send_xon = true;
	send_xoff = false;

	/////////// GET HERE /////////////////////

	/* Create child process */
	/*pid_t pid = fork(); */
	pthread_t child_thread;
	// (void *)
	int rc = pthread_create(&child_thread, NULL, childProcess, (void *)0);
	if(rc){
		printf("Error:unable to create thread %d\n", rc);
        exit(-1);
	}

	/*** IF PARENT PROCESS ***/
	while(true){
		c = *(rcvchar(sockfd, rxq));
		// Quit on end of file
		if(c == Endfile){
			exit(0);
		}
	}
	/*** ELSE IF CHILD PROCESS ***/ // udah ditaro diatas

	pthread_exit(NULL);
	return 0;
}

static Byte *rcvchar(int sockfd, QTYPE *q){

	//gayakin sama ini plis
	char c[10];
	recvfrom(sockfd, c, 1, 0, (struct sockaddr*)&serv_addr , &addrlen);
	q->count++;
	q->data[q->rear] = c[0];
	q->rear++;

	if(q->rear == RXQSIZE) {
		q->rear = 0;
	}

	if(q->count >= 10 && !send_xoff){
		send_xoff = true;
		char kirimXOFF = XOFF;
		printf("Buffer > minimum upperlimit.\n");
		sendto(sockfd, (char*)&kirimXOFF, 1, 0, (struct sockaddr*)&serv_addr , sizeof(addrlen));
	}
}
	/*
	 * Insert code here.
	 * Read a character from socket and put it to the receive buffer.
	 * If the number of characters in the receive buffer is above
	 * certain level, then send XOFF and set a flag (why?).
	 * Return a pointer to the buffer where data is put.
	 */
/* q_get returns a pointer to the buffer where data is read
 * or NULL if buffer is empty/
 */
static Byte *q_get(QTYPE *q){
	Byte *current;
	/* Nothing in the queue */
	if(!q->count) return (NULL);
	q->count--;
	current = &(q->data[q->front]);
	q->front++;

	if(q->front == RXQSIZE){
		q->front = 0;
	}

	if(q->count <= 4 && !send_xon){
		send_xon = true;
		char kirimXON = XON;
		puts("Buffer < maximum lowerlimit.");
		sendto(sockfd, (char*)&kirimXON, 1, 0, (struct sockaddr*)&serv_addr , sizeof(addrlen));
	}

	/*
	 * Insert code here.
	 * Retrieve data from buffer, save it to "current"
	 * If the number of characters in the recieve buffer is below
	 * certain level, then send XON.
	 * Increment front index and check for wraparound.
	 */
	 return current;
}
