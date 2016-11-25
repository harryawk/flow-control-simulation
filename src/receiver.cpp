/*
 * File : T1_rx.cpp
 */
#include "dcomm.h"
#include "support.h"
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
#include "support.h"

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 16
#define WINDOWSIZE 6

int window[WINDOWSIZE];
int msgno = 0;

MESGB messages[RXQSIZE];

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
static MESGB *rcvframe(int sockfd, QTYPE *q);

struct sockaddr_in serv_addr, cli_addr;
socklen_t addrlen = sizeof(serv_addr), clilen = sizeof(cli_addr);
int byte_idx = 0;

/* Child process, read character from buffer */
void *childProcess(void *threadid){
	int byte_now = 0;

	while(true){
		Byte *now;
		now = q_get(rxq);
		if(now != NULL){
			printf("Mengkonsumsi byte ke-%d: '%c'\n", ++byte_now, *now);
			usleep(DELAY * 1000);
		}
	}
	pthread_exit(NULL);
}

// SERVER PROGRAM
int main(int argc, char *argv[]){
	// Byte c;
	MESGB c;
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

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { //binding error
   		perror("ERROR : on binding");
   		exit(1);
   	}
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &serv_addr.sin_addr, clientName, sizeof (clientName));
	printf("Binding pada %s:%d\n", clientName, ntohs(serv_addr.sin_port));

	/* Initialize XON/XOFF flags */
	send_xon = true;
	send_xoff = false;

	/* Create child process */
	pthread_t child_thread;
	// (void *)
	int rc = pthread_create(&child_thread, NULL, childProcess, (void *)0);
	if(rc){ //failed creating thread
		printf("Error:unable to create thread %d\n", rc);
        exit(-1);
	}
	/*** IF PARENT PROCESS ***/
	while(true){
		// c = *(rcvchar(sockfd, rxq));
		c = *(rcvframe(sockfd, rxq));
		msgno++;
		// Quit on end of file
		if(c.data[0] == Endfile){
			exit(0);
		}
	}
	pthread_join(child_thread, NULL);
	pthread_exit(NULL);
	return 0;
}

/* function for reading character and put it to the receive buffer*/
static Byte *rcvchar(int sockfd, QTYPE *q){

	Byte *cur;
	char c[10];
	// MESGB c;
	int byte_recv = recvfrom(sockfd, c, 1, 0, (struct sockaddr*)&cli_addr, &clilen);
	if(byte_recv < 0){ //error receiving character
		printf("Error receiving: %d", byte_recv);
		exit(-2);
	}
/////////////////////////////////////
	// cek struktur frame

	// cek checksum

	// succeed, sendto(transmitter, ACK);

////////////////////////////////////
	// succeed reading character
	q->count++;
	q->data[q->rear] = c[0];
	cur = &(q->data[q->rear]);
	q->rear++;
	if(q->rear == RXQSIZE) {
		q->rear = 0;
	}

	printf("Menerima byte ke-%d.\n", ++byte_idx);

	// receive buffer above minimum upperlimit
	// sending XOFF
	if(q->count >= 8 && !send_xoff){
		send_xoff = true;
		send_xon = false;
		printf("Buffer > minimum upperlimit.\n");
		printf("Mengirim XOFF\n");
		sendto(sockfd, kirimXOFF, 1, 0, (struct sockaddr*)&cli_addr, clilen);
	}
	return cur;
}

/* q_get returns a pointer to the buffer where data is read
 * or NULL if buffer is empty
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

	// receive buffer below maximum lowerlimit
	// sending XON
	if(q->count < 4 && !send_xon){
		send_xon = true;
		send_xoff = false;
		printf("Buffer < maximum lowerlimit.\n");
		printf("Mengirim XON\n");
		int x = sendto(sockfd, (char*)&kirimXON, 1, 0, (struct sockaddr*)&cli_addr , clilen);
		if (x < 0) {
			printf("error: wrong socket\n");
			exit(-3);
		}
	}
	return current;
}

static MESGB *rcvframe(int sockfd, QTYPE *q){
	MESGB *cur;
	MESGB c[RXQSIZE];
	int byte_recv = recvfrom(sockfd, c, sizeof(MESGB), 0, (struct sockaddr*)&cli_addr, &clilen);
	if(byte_recv < 0){ //error receiving character
		printf("Error receiving: %d", byte_recv);
		exit(-2);
	}
/////////////////////////////////////

	// cek struktur frame

	if (c[0].soh != SOH || c[0].stx != STX || c[0].etx != ETX || c[0].msgno != msgno) {
		printf("kirim NAK\n");
		exit(-3);
	}

	// cek checksum

	unsigned char checksumarr[5];
	checksumarr[0] = c[0].soh;
	checksumarr[1] = c[0].stx;
	checksumarr[2] = c[0].etx;
	checksumarr[3] = c[0].msgno;
	checksumarr[4] = c[0].data[0];
	unsigned int checksum = crc32a(&checksumarr);
	if (checksum != c[0].checksum) {
		printf("kirim NAK\n");
		exit(-4);
	}

	// succeed, sendto(transmitter, ACK);

	int send_ack = sendto(sockfd, ACK, sizeof(ACK), 0, (struct sockaddr*)&cli_addr, clilen);
	if(send_ack < 0){ //error sending ACK character
		printf("Error send ACK: %d", send_ack);
		exit(-2);
	}
////////////////////////////////////
	// succeed reading character
////////////////////////////////////
	q->count++;
	q->data[q->rear] = c[0].data;
	cur.data = &(q->data[q->rear]);
	q->rear++;
	if(q->rear == RXQSIZE) {
		q->rear = 0;
	}

	printf("Menerima frame ke-%d.\n", ++byte_idx);

	// receive buffer above minimum upperlimit
	// sending XOFF
	if(q->count >= 8 && !send_xoff){
		send_xoff = true;
		send_xon = false;
		printf("Buffer > minimum upperlimit.\n");
		printf("Mengirim XOFF\n");
		sendto(sockfd, kirimXOFF, 1, 0, (struct sockaddr*)&cli_addr, clilen);
	}
	return cur;
}
