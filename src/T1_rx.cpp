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
bool send_xon = false, send_xoff = false;

/* Socket */
int sockfd; //listen on sock_fd

char kirimXOFF[10];
char kirimXON[10];
/* Functions declaration */
static Byte *rcvchar(int, QTYPE *);
static Byte *q_get(QTYPE *);

struct sockaddr_in serv_addr, cli_addr;
socklen_t addrlen = sizeof(serv_addr), clilen = sizeof(cli_addr);
int byte_idx = 0;
void *childProcess(void *threadid){
	int byte_now = 0;
	struct timespec t_per_recv;
	t_per_recv.tv_sec = 1;
	t_per_recv.tv_nsec = 0;

	while(true){
		Byte *now;
		now = q_get(rxq);
		if(now != NULL){
			printf("Mengkonsumsi byte ke-%d: '%c'\n", ++byte_now, *now);
			nanosleep(&t_per_recv, NULL);
		}

		/* Call q_get */
		/* Can introduce some delay here. */
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
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// initialize server address
	kirimXOFF[0] = (char) XOFF;
	kirimXOFF[1] = '\0';
	kirimXON[0] = (char) XON;
	kirimXON[1] = '\0';

   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
   	if (argc > 2) {
   		serv_addr.sin_port = htons(atoi(argv[2]));
   	} else {
   		serv_addr.sin_port = htons(13514);
   	}
   	//bzero((char *) &serv_addr, sizeof(serv_addr));
   	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
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
	pthread_join(child_thread, NULL);
	pthread_exit(NULL);
	return 0;
}

static Byte *rcvchar(int sockfd, QTYPE *q){

	Byte *cur;
	char c[10];
	int byte_recv = recvfrom(sockfd, c, 1, 0, (struct sockaddr*)&cli_addr, &clilen);
	if(byte_recv < 0){
		printf("Error receiving: %d", byte_recv);
		// perror(recvfrom);
		exit(-2);
	}
	q->count++;
	q->data[q->rear] = c[0];
	cur = &(q->data[q->rear]);
	q->rear++;
	if(q->rear == RXQSIZE) {
		q->rear = 0;
	}

	printf("Menerima byte ke-%d.\n", ++byte_idx);
	if(q->count >= 8 && !send_xoff){
		send_xoff = true;
		send_xon = false;
		printf("Buffer > minimum upperlimit.\n");
		printf("Mengirim XOFF\n");
		sendto(sockfd, kirimXOFF, 1, 0, (struct sockaddr*)&cli_addr, clilen);
	}
	return cur;
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
	(q->count)--;
	current = &(q->data[q->front]);
	q->front++;

	if(q->front == RXQSIZE){
		q->front = 0;
	}

	if(q->count < 4 && !send_xon){
		send_xon = true;
		send_xoff = false;
		printf("Buffer < maximum lowerlimit.\n");
		printf("Mengirim XON\n");
		sendto(sockfd, kirimXON, 1, 0, (struct sockaddr*)&cli_addr, clilen);
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
