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
using namespace std;

bool xoff = false;
char receiverAddress[1000];
struct sockaddr_in serv_addr, cli_addr;
socklen_t serv_len = sizeof(serv_addr), cli_len = sizeof(cli_addr);
int sockfd;
bool done = false;

/* Child process, receiving XON/XOFF signal from receiver*/
void *childProcess(void *threadid){
	int now = 0;
	struct timespec t_per_recv;
	t_per_recv.tv_sec = 0;
	t_per_recv.tv_nsec = 100000000;

	while(!done){
		// child receieve xon/xoff signal
		char c[10];
		memset(c, 0,sizeof c);
		int rc = recvfrom(sockfd, c, 1, 0, (struct sockaddr*) &serv_addr, &serv_len);
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

	char cc;
	int idx = 0;
	FILE* myfile = fopen(argv[3], "r");
	// reading from file
	while (!feof(myfile)){
		if(!xoff){
			fscanf(myfile, "%c", &cc);
			idx++;
			printf("Mengirim byte ke-%d: \'%c\' \n", idx, cc);
			// send character from file to socket
			sendto(sockfd, (char*)&cc, 1, 0, (struct sockaddr*)&serv_addr, serv_len);
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
