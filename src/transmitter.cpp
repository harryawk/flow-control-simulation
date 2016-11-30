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
#include <sys/time.h>
#include "dcomm.h"
#include "support.h"
using namespace std;

//build a socket
struct sockaddr_in serv_addr, cli_addr;
socklen_t serv_len = sizeof(serv_addr), cli_len = sizeof(cli_addr);
char receiverAddress[MAXLEN + 40];
int sockfd;
int NAKnum = -1;

//buffer to sendto and recvfrom
char c_recvfrom[MAXLEN  + 40], c_sendto[MAXLEN + 40];

MESGB mesg;

//flow control protocol
bool xoff = false;
bool done = false;

//sliding window protocol
char cc[RXQSIZE][MAXLEN + 10]; //size of buffer in chunks
char lastacked = -1, lastsent = -1, countBuf = 0;

bool corruptACK(char* s);
void createSocket(char* addr, char* port);
void initMESGB();
void initTimeOut();
string convMESGBtostr(MESGB m);

//child process
void *childProcess(void *threadid);

int main(int argc, char *argv[]){
	/* Create socket */
	if(argc != 4){
		printf("Number of arguments should be 4!");
		exit(-1);
	}
	createSocket(argv[1], argv[2]);
	initMESGB();
	initTimeOut();

	/* Create child process for receiving data*/
	pthread_t child_thread;
	int rc = pthread_create(&child_thread, NULL, childProcess, 0);
	if(rc){ //failed creating thread
		printf("Error:unable to create thread %d\n", rc);
		exit(-1);
	}

	// reading from file
	FILE* myfile = fopen(argv[3], "r");
	int idx = 0;

	/* Parent send data */
	while (!feof(myfile) || countBuf > 0){
		printf("MASUK %d %d %d\n", xoff, NAKnum, countBuf);
		if(!feof(myfile) && !xoff && NAKnum == -1 && countBuf < 13){
			fgets(cc[(lastsent + 1) % RXQSIZE], MAXLEN, myfile); //read MAXLEN character from file

			// last sent frame number
			lastsent = (lastsent + 1) % RXQSIZE;

			// banyak yang belum terkirim
			countBuf = (lastsent - lastacked);
			countBuf %= RXQSIZE;
			if(countBuf < 0) countBuf += RXQSIZE;

			// frame data
			mesg.msgno = idx % RXQSIZE;
			mesg.data = cc[idx % RXQSIZE];
			string s = convMESGBtostr(mesg);

			// prepare c_sendto buffer to be sent to receiver
			memset(c_sendto, 0, sizeof c_sendto);
			for(int i = 0;i < s.length();++i){
				c_sendto[i] = s[i];
			}

			printf("Mengirim frame ke-%d: \'%s\' \n", idx, cc[lastsent]);
			idx++;
			sendto(sockfd, c_sendto, sizeof(c_sendto), 0, (struct sockaddr*)&serv_addr, serv_len);
			usleep(100000);
		}
		else if(xoff && NAKnum == -1){ // XOFF sent, receive buffer is above minimum upperlimit
			printf("Menunggu XON...\n");
			usleep(200000);
		}
		else{ // !xoff or NAKnum != -1
			if(NAKnum == -1) NAKnum = (lastacked + 1) % NAKnum;
			mesg.msgno = NAKnum;
			mesg.data = cc[NAKnum % RXQSIZE];
			string s = convMESGBtostr(mesg);

			memset(c_sendto, 0, sizeof c_sendto);
			for(int i = 0;i < s.length();++i){
				c_sendto[i] = s[i];
			}

			printf("Mengirim NAK ke-%d: \'%s\' \n", NAKnum, cc[NAKnum]);
			sendto(sockfd, c_sendto, sizeof(c_sendto), 0, (struct sockaddr*)&serv_addr, serv_len);
			usleep(200000);
			NAKnum = -1;
		}
	}
	done = true;
	// reach end of file
	fclose(myfile);
	exit(0);
}

bool corruptACK(char* s){
	int i = 0;
	string ss = "";
	unsigned int checksum = 0;
	unsigned int real_c = 0;

	while(s[i] != 0 || i < 2){
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
			unsigned char ta[MAXLEN + 40];
			memset(ta, 0, sizeof ta);
			for(int j = 0; j < ss.length(); ++j){
				ta[j] = ss[j];
			}
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

void createSocket(char* addr, char* port){
	//define a client socket
	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(addr);
   	serv_addr.sin_port = htons(atoi(port));

   	inet_ntop(AF_INET, &serv_addr.sin_addr, receiverAddress, sizeof (receiverAddress));

	printf("Membuat socket untuk koneksi ke %s:%d ...\n", receiverAddress, htons(serv_addr.sin_port));
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
}

//initialize SOH, STX, and ETX value of frame with defined value SOH, STX, dan ETX
void initMESGB(){
	mesg.soh = SOH;
	mesg.stx = STX;
	mesg.etx = ETX;
}

//initialize timeout value
void initTimeOut(){
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 000000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}
}

//child process, receiving XON/XOFF and ACK/NAK signal from receiver
void *childProcess(void *threadid){

	int now = 0;
	while(!done){
		memset(c_recvfrom, 0,sizeof c_recvfrom);
		int rc = recvfrom(sockfd, c_recvfrom, sizeof(c_recvfrom), 0, (struct sockaddr*) &serv_addr, &serv_len);
		if(rc < 0){
			printf("Timeout!\n");
			NAKnum = (lastacked + 1) % RXQSIZE;
			printf("NAK Timeout %d\n", NAKnum);
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
			if(!corruptACK(c_recvfrom)){ //receive ACK/NAK
				if(c_recvfrom[0] == ACK){ //receive ACK
					printf("ACK %d\n", c_recvfrom[1]);
					lastacked = c_recvfrom[1];
					if(lastacked <= lastsent){
						countBuf = lastsent - lastacked;
					}
					else{
						countBuf = lastsent + 1 + (RXQSIZE - 1) - lastacked;
					}
				}
				else{ //receive NAK
					NAKnum = c_recvfrom[1];
					printf("NAK %d\n", NAKnum);
				}
			}
			else{ //data corrupted
				puts("CORRUPT!!!!");
			}
		}
	}
}

//convert frame to string and add checksum value
string convMESGBtostr(MESGB m){
	string ret = "";
	ret += m.soh;
	ret += m.msgno;
	ret += m.stx;
	ret += m.data;
	ret += m.etx;
	unsigned char t[MAXLEN + 40]; //for checksum
	memset(t,0,sizeof t);
	for(int i = 0;i < ret.length(); ++i){
		t[i] = ret[i];
	}
	m.checksum = crc32a(t);
	ret += to_string(m.checksum);
	return ret;
}
