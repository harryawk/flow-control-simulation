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
#include "dcomm.h"
#include "support.h"
using namespace std;

bool xoff = false;
char receiverAddress[MAXLEN];
struct sockaddr_in serv_addr, cli_addr;
socklen_t serv_len = sizeof(serv_addr), cli_len = sizeof(cli_addr);
int sockfd;
bool done = false;
char c_recvfrom[MAXLEN << 1], c_sendto[MAXLEN << 1];
Byte cc[RXQSIZE];
Byte lastacked = RXQSIZE - 1, lastsent = 0, countBuf = 0;

bool corruptACK(char* s){
	int i = 0;
	string ss = "";
	unsigned int checksum = 0;
	unsigned int real_c = 0;
	while(s[i] != 0){
		if(i == 0){
			if(s[i] != ACK && s[i] != NAK){
				return true;
			}
			else{
				ss += s[i];
			}
		}
		else if(i == 1){
			ss += s[i];
			unsigned char ta[MAXLEN];
			strcpy( (char*) ta, ss.c_str());
			checksum = crc32a(ta);
		}
		else{
			real_c *= 10;
			real_c += s[i] - '0';
		}
		++i;
	}
	return checksum != real_c;
}

/* Child process, receiving XON/XOFF signal from receiver*/
void *childProcess(void *threadid){
	int now = 0;
	struct timespec t_per_recv;
	t_per_recv.tv_sec = 0;
	t_per_recv.tv_nsec = 100000000;

	while(!done){
		// child receieve xon/xoff signal
		memset(c_recvfrom, 0,sizeof c_recvfrom);
		int rc = recvfrom(sockfd, c_recvfrom, sizeof(c_recvfrom), 0, (struct sockaddr*) &serv_addr, &serv_len);

		if(rc < 0){
			printf("Error receiving byte: %d", rc);
			exit(-2);
		}
		if(c_recvfrom[0] == XON){
			printf("XON diterima.\n");
			xoff = false;
		}
		else if(c_recvfrom[0] == XOFF){
			printf("XOFF diterima.\n");
			xoff = true;
		}
		else{
			//ngecek checksum dulu
			if(!corruptACK(c_recvfrom)){
				//dia nerima ACK / NAK
				if(c_recvfrom[0] == ACK){
					lastacked = c_recvfrom[1];
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
					mesg.msgno = c_recvfrom[1];
					mesg.data = &cc[c_recvfrom[1]];
					string s = (char)mesg.soh + (char)mesg.msgno + (char)mesg.stx + (char*)mesg.data + (char)mesg.etx;
					unsigned char t[MAXLEN];
					strcpy( (char*) t, s.c_str());
					mesg.checksum = crc32a(t);
					s += to_string(mesg.checksum);
					strcpy(c_sendto, s.c_str());
					sendto(sockfd, c_sendto, sizeof(c_sendto), 0, (struct sockaddr*)&serv_addr, serv_len);
					memset(c_sendto, 0, sizeof c_sendto);
				}
			}
		}
	}
}

int main(int argc, char *argv[]){

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
			fscanf(myfile, "%c", &cc[(lastsent + 1) % RXQSIZE]);
			lastsent = (lastsent + 1) % RXQSIZE;


			MESGB mesg;
			mesg.soh = SOH;
			mesg.stx = STX;
			mesg.etx = ETX;
			mesg.msgno = idx;
			mesg.data = &cc[idx % RXQSIZE];
			string s = (char)mesg.soh + (char)mesg.msgno + (char)mesg.stx + (char*)mesg.data + (char)mesg.etx;
			unsigned char t[MAXLEN];
			strcpy( (char*) t, s.c_str());
			mesg.checksum = crc32a(t);
			s += to_string(mesg.checksum);
			strcpy(c_sendto, s.c_str());

			// send character from file to socket
			idx++;
			printf("Mengirim byte ke-%d: \'%c\' \n", idx, cc[(lastsent + 1) % RXQSIZE]);

			sendto(sockfd, &c_sendto, sizeof(c_sendto), 0, (struct sockaddr*)&serv_addr, serv_len);
			memset(c_sendto, 0, sizeof c_sendto);
			usleep(100000);
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
