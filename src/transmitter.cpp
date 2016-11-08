#include "dcomm.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

using namespace std;


int main(int argc, char *argv[] ){

	/* Create socket */
	struct sockaddr_in serv_addr;
	socklen_t addrlen = sizeof(serv_addr);

	int serversock = socket(AF_INET, SOCK_DGRAM, 0);
	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
   	if (argc > 2) {
   		serv_addr.sin_port = htons(atoi(argv[2]));
   	} else {
   		serv_addr.sin_port = htons(13514);
   	}

   	if (bind(serversock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
   		perror("ERROR : on binding");
   		exit(1);
   	}
   	// if get out of here : serversock binded
	inet_ntop(AF_INET, &serv_addr.sin_addr, clientName, sizeof (clientName));

	printf("Membuat socket untuk koneksi ke %s:%s ...\n", argv[1], argv[2]);

	/* Create child process */

	// child receieve xon/xoff signal
		printf("XOFF diterima.\n");

		printf("Menunggu XON...\n");

		printf("XON diterima.\n");

	// parent send data

	int i=1;
	char cc;
	FILE* myfile = fopen(argv[3], "r"];
	while (!myfile.eof()){
		fscanf(myfile, "%c", &cc);
		printf("Mengirim byte ke-%d: \'%c\' \n", i, cc);
		i++;
		sleep(0.1);
	}
	fclose(myfile);




	return 0;
}
