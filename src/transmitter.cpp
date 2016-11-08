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

void *childProcess(void *threadid){
	int byte_now = 0;
	while(true){
		// child receieve xon/xoff signal
		char c[10];
		recvfrom(sockfd, c, 1, 0, (struct sockaddr*) &serv_addr, &serv_len);
		if(c[0] == XOFF){
			printf("XOFF diterima.\n");
			xoff = true;
		}
		else if(c[0] == XON){
			printf("XON diterima.\n");
			xoff = false;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[] ){

	/* Create socket */


	//define a client socket
	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
   	serv_addr.sin_port = htons(atoi(argv[2]));

   	// if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
   	// 	perror("ERROR : on binding");
   	// 	exit(1);
   	// }
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &cli_addr.sin_addr, receiverAddress, sizeof (receiverAddress));

	printf("Membuat socket untuk koneksi ke %s:%d ...\n", receiverAddress, htons(serv_addr.sin_port));
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Create child process */

	pthread_t child_thread;
	// (void *)
	int rc = pthread_create(&child_thread, NULL, childProcess, 0);
	if(rc){
		printf("Error:unable to create thread %d\n", rc);
        exit(-1);
	}
	// parent send data

	char cc;
	int idx = 0;
	FILE* myfile = fopen(argv[3], "r");
	while (!feof(myfile)){
		fscanf(myfile, "%c", &cc);
		printf("Mengirim byte ke-%d: \'%c\' \n", idx, cc);
		idx++;
		sendto(sockfd, (char*)&cc, 1, 0, (struct sockaddr*)&serv_addr, serv_len);
		sleep(0.1);
		while(xoff){
			printf("Menunggu XON...\n");
			sleep(2);
		}
	}
	fclose(myfile);
	return 0;
}
