/*
 * File : T1_rx.cpp
 */

#include "dcomm.h"
#include <stdlib.h>
#include <stddef.h>
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 8

Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false, send_xoff = false;

/* Socket */
int sockfd; //listen on sock_fd

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);

int main(int argc, char *argv[]){
	Byte c;
	
	/*
	 * Insert code here to bind socket to the port number given in argv[1]
	 */
	
	/* Initialize XON/XOFF flags */
	
	/* Create child process */
	
	/*** IF PARENT PROCESS ***/
		while(true){
			c = *(rcvchar(sockfd, rxq));
			
			/* Quit on end of file */
			if (c == Endfile){
				exit(0);
			}
		}
	/*** ELSE IF CHILD PROCESS ***/
		while(true){
			/* Call q_get */
			/* Can introduce some delay here. */
		}
}

static Byte *rcvchar(int sockfd, QTYPE *queue){
	/*
	 * Insert code here.
	 * Read a character from socket and put it to the receive buffer.
	 * If the number of characters in the receive buffer is above
	 * certain level, then send XOFF and set a flag (why?).
	 * Return a pointer to the buffer where data is put.
	 */
}

/* q_get returns a pointer to the buffer where data is read 
 * or NULL if buffer is empty/
 */
static Byte *q_get(QTYPE *queue, Byte *data){
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
