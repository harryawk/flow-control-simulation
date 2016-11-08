#include "dcomm.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

using namespace std;

int main(int argc, char *argv[] ){

	/* Create socket */
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
	while (!eof()){
		fscanf(myfile, %c, &cc);
		printf("Mengirim byte ke-%d: \'%c\' \n", i, cc);
		i++;
	}
	fclose(myfile);



	return 0;
}
