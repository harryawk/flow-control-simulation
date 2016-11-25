/*
 * File : receiver.cpp
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
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <stdio.h>
#include <pthread.h>
#include "support.h"
using namespace std;

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 20
/* Define receive buffer size */
#define RXQSIZE 16
#define WINDOWSIZE 10

#define fi first
#define se second

/////////////////////////////////////////////////////////////////
int msgno = 0;
/* for sliding window protocol and selective-repeat ARQ */
Byte rxbuf[RXQSIZE];
Byte msg[RXQSIZE][MAXLEN + 10];
char clientName[MAXLEN];
char recvbuf[MAXLEN << 1], sendbuf[MAXLEN << 1];

QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;

/* for send xon and xoff */
char sendxonxoff = XON;
bool send_xon = false, send_xoff = false;
int lastrecv = 0, lastacked = 0;

/* Socket */
int sockfd; //listen on sock_fd
void createSocket(char* addr, char* port);

/* Functions declaration */
static Byte* q_get(QTYPE *);
static int rcvframe(int sockfd, QTYPE *q);
pair<int, string> convbuf(char* c);
string convRESPtostr(int res, int msgno);

struct sockaddr_in serv_addr, cli_addr;
socklen_t addrlen = sizeof(serv_addr), clilen = sizeof(cli_addr);

/* Child process, read character from buffer */
void *childProcess(void *threadid);
void sendACK(int framenum);
void sendNAK(int framenum);

// SERVER PROGRAM
int main(int argc, char *argv[]){
	// create socket
	createSocket(argv[1], argv[2]);

	/* Initialize XON/XOFF flags */
	send_xon = true;
	send_xoff = false;

	/* Create child process */
	pthread_t child_thread;
	int rc = pthread_create(&child_thread, NULL, childProcess, (void *)0);
	if(rc){ //failed creating thread
		printf("Error:unable to create thread %d\n", rc);
        exit(-1);
	}
	
	/*** IF PARENT PROCESS ***/
	while(true){
		int ret = rcvframe(sockfd, rxq);
	}

	pthread_join(child_thread, NULL);
	pthread_exit(NULL);
	return 0;
}

/* function for reading character and put it to the receive buffer*/

/* q_get returns a pointer to the buffer where data is read
 * or NULL if buffer is empty
 */
static Byte* q_get(QTYPE *q){
	Byte* current;
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
		sendxonxoff = XON;
		printf("Buffer < maximum lowerlimit.\n");
		printf("Mengirim XON\n");
		int x = sendto(sockfd, &sendxonxoff, 1, 0, (struct sockaddr*)&cli_addr , clilen);
		if (x < 0) {
			printf("error: wrong socket\n");
			exit(-3);
		}
	}
	return current;
}

static int rcvframe(int sockfd, QTYPE *q){
	memset(recvbuf, 0, sizeof recvbuf);
	puts("Pi*pin Masuk");
	int byte_recv = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&cli_addr, &clilen);
	puts("Pi*pin Keluar");
	if(byte_recv < 0){ //error receiving character
		printf("Error receiving: %d", byte_recv);
	}
	pair<int,string> M = convbuf(recvbuf);
	if(M.se != ""){ // dia ga error
		printf("Menerima frame ke-%d.\n", M.fi);
		// receive buffer above minimum upperlimit
		// sending XOFF

		//bagi 2, apakah dia ada di depan lastacked apa dibelakang lastacked.
		//intinya bagi duanya itu buat mau diabaikan apa diterima
		if(M.fi <= q->rear){
			if(M.fi >= q->front){
				strcpy((char*)msg[M.fi], M.se.c_str());
			}
			else if(q->front > q->rear){
				strcpy((char*)msg[M.fi], M.se.c_str());
			}
			else{
				//bagi dua lagi, apakah ada didalem batas apa nggak, ini ditulis soalnya bisa aja dia sebenernya lanjutannya tapi udah muter
				if((M.fi + 1) + RXQSIZE - q->front < WINDOWSIZE){
					q->count = M.fi + 1 + RXQSIZE - q->front;
					q->rear = M.fi;
					strcpy((char*)msg[q->rear], M.se.c_str());
					lastrecv = q->rear;
				}
				else {
					//do nothing!
				}
			}
		}
		else{
			if(q->front > q->rear){
				//bagi dua lagi, lewat bates apa nggak
				if(M.fi + 1 + RXQSIZE - q->front < WINDOWSIZE + 3){
					q->count = M.fi + 1 + RXQSIZE - q->front;
					q->rear = M.fi;
					strcpy((char*)msg[q->rear], M.se.c_str());
					lastrecv = q->rear;
				}
			}
			else{
				//bagi dua lagi, lewat bates apa nggak
				if(M.fi - q->front + 1 < WINDOWSIZE + 3){
					q->count = M.fi - q->front + 1;
					q->rear = M.fi;
					strcpy((char*)msg[q->rear], M.se.c_str());
					lastrecv = q->rear;
				}
			}
		}

		if(q->count >= WINDOWSIZE){
			sendxonxoff = XOFF;
			send_xoff = true;
			send_xon = false;
			printf("Buffer > minimum upperlimit.\n");
			printf("Mengirim XOFF\n");
			sendto(sockfd, &sendxonxoff, 1, 0, (struct sockaddr*)&cli_addr, clilen);
		}
	}
	else{
		sendNAK(q->front);
	}
	return M.fi;
}

void createSocket(char* addr, char* port){
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// initialize server address
   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(addr);
   	serv_addr.sin_port = htons(atoi(port));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { //binding error
   		perror("ERROR : on binding");
   		exit(1);
   	}
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &serv_addr.sin_addr, clientName, sizeof (clientName));
	printf("Binding pada %s:%d\n", clientName, ntohs(serv_addr.sin_port));
}

pair<int, string> convbuf(char* buf){
	pair<int,string> res = make_pair(-1,"");
	if(buf[0] != SOH) return res;
	if(buf[2] != STX) return res;
	res.fi = buf[1];
	int idx = 0;
	res.se += buf[0] + buf[1] + buf[2];
	for(idx = 3;buf[idx] != ETX && idx < MAXLEN + 15; ++idx){
		res.se += buf[idx];
	}
	res.se += buf[idx];
	++idx;
	unsigned int check = 0;
	while(buf[idx] != 0){
		check *= 10;
		check += buf[idx] - '0';
	}
	unsigned char t[MAXLEN << 1]; //for checksum
	strcpy( (char*) t, (res.se).c_str());
	unsigned int checksum = crc32a(t);
	if(checksum != check) return make_pair(-1, "");
	return res;
}

string convRESPtostr(int res, int msgno){
	RESP R;
	R.res = res;
	R.msgno = msgno;
	unsigned char* uc = new unsigned char[3];
	uc[0] = R.res;
	uc[1] = R.msgno;
	R.checksum = crc32a(uc);
	memset(sendbuf, 0, sizeof sendbuf);
	string s = "";
	s += (char)R.res;
	s += (char)R.msgno;
	s += to_string(R.checksum);
	strcpy(sendbuf, s.c_str());
}

void *childProcess(void *threadid){
	int byte_now = 0;
	while(true){
		Byte* now;
		//ini masih harus diubah lagi, melihat dia getnya ga berurutan
		now = q_get(rxq);
		if(now != NULL){
			printf("Mengkonsumsi byte ke-%d.\n", ++byte_now);
			sendACK(*now);
			usleep(DELAY * 1000);
		}
		else{
			if(rxq->count != 0){
				sendNAK(rxq->front);
			}
		}
	}
	pthread_exit(NULL);
}

void sendACK(int framenum){
	string s = convRESPtostr(ACK, framenum);
	int send_ack = sendto(sockfd, sendbuf, sizeof(sendbuf), 0, (struct sockaddr*)&cli_addr, clilen);
	if(send_ack < 0){//error sending ACK character
		printf("Error send ACK: %d", send_ack);
	}
}

void sendNAK(int framenum){
	string s = convRESPtostr(NAK, framenum);
	int send_ack = sendto(sockfd, sendbuf, sizeof(sendbuf), 0, (struct sockaddr*)&cli_addr, clilen);
	if(send_ack < 0){//error sending ACK character
		printf("Error send NAK: %d", send_ack);
	}
}
