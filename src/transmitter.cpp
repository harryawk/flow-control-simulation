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
#include "support.h"
using namespace std;

bool xoff = false;
char receiverAddress[1000];
struct sockaddr_in serv_addr, cli_addr;
socklen_t serv_len = sizeof(serv_addr), cli_len = sizeof(cli_addr);
int sockfd;
bool done = false;
char c[MAXLEN << 1];
char cc[RXQSIZE];
Byte lastacked = RXQSIZE - 1, lastsent = 0, countBuf = 0;

/* Child process, receiving XON/XOFF signal from receiver*/
void *childProcess(void *threadid){
	int now = 0;
	struct timespec t_per_recv;
	t_per_recv.tv_sec = 0;
	t_per_recv.tv_nsec = 100000000;

	while(!done){
		// child receieve xon/xoff signal
		memset(c, 0,sizeof c);
		int rc = recvfrom(sockfd, c, sizeof(c), 0, (struct sockaddr*) &serv_addr, &serv_len);

		if(rc < 0){
			printf("Error receiving byte: %d", rc);
			exit(-2);
		}
		if(c[0] == XON){
			printf("XON diterima.\n");
			xoff = false;
		}
		else if(c[0] == XOFF){
			printf("XOFF diterima.\n");
			xoff = true;
		}
		else{
			//ngecek checksum dulu

			//dia nerima ACK / NAK
			if(c[0] == ACK){
				lastacked = c[1];
				if(lastacked <= lastsent){
					countBuf = lastsent - lastacked;
				}
				else{
					countBuf = lastsent + 1 + (RXQSIZE - 1) - lastacked;
				}
			}
			else{
				MESGB mesg;
				mesg.soh = SOH;
				mesg.stx = STX;
				mesg.etx = ETX;
				mesg.msgno = c[1];
				mesg.data = &cc[c[1]];
				string s = (char)mesg.soh + (char)mesg.mesgno + (char)mesg.stx + *mesg.data + mesg.etx;
				mesg.checksum = crc32a(s);
				s += mesg.checksum;
				strcpy(c, s.c_str());
				sendto(sockfd, c, sizeof(c), 0, (struct sockaddr*)&serv_addr, serv_len);
			}
		}
	}
}

int main(int argc, char *argv[] ){

	/* Create socket */

	//define a client socket
	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
   	serv_addr.sin_port = htons(atoi(argv[2]));

   	inet_ntop(AF_INET, &serv_addr.sin_addr, receiverAddress, sizeof (receiverAddress));

	printf("Membuat socket untuk koneksi ke %s:%d ...\n", receiverAddress, htons(serv_addr.sin_port));
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Create child process */

	pthread_t child_thread;

	int rc = pthread_create(&child_thread, NULL, childProcess, 0);
	if(rc){ //failed creating thread
		printf("Error:unable to create thread %d\n", rc);
        exit(-1);
	}
	/* Parent send data */

	int idx = 0;
	FILE* myfile = fopen(argv[3], "r");
	// reading from file
	while (!feof(myfile)){
		if(!xoff){
			fscanf(myfile, "%c", &cc);
			idx++;
			printf("Mengirim byte ke-%d: \'%c\' \n", idx, cc);
			// send character from file to socket
			sendto(sockfd, (char*)&cc, sizeof(cc), 0, (struct sockaddr*)&serv_addr, serv_len);
			struct timespec t_per_send, t_xon;
			t_per_send.tv_sec = 0;
			t_per_send.tv_nsec = 100000000;
			nanosleep(&t_per_send, NULL);
		}
		else{ // XOFF sent, receive buffer is above minimum upperlimit
			printf("Menunggu XON...\n");
			sleep(2);
		}
	}
	// reach end of file
	fclose(myfile);
	exit(0);
}
