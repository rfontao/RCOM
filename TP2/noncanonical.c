/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "message_macros.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

void readPort(int fd, char* data) {

	int a = 0;
	int res = 0;
	char* result = (char*)malloc(sizeof(char));

	while (STOP == FALSE) {
		res = read(fd, result, 1);
		if(a == 0)
			strcpy(data, result);
		else
			strcat(data, result);
		a++;
		if (result[0] == FLAG)
			STOP = TRUE;
	}
	STOP = FALSE;

	printf("%d bytes received\n", res);
}

int main(int argc, char **argv){
	int fd, res;
	struct termios oldtio, newtio;
	char buf[255];

	if ((argc < 2) ||
		((strcmp("/dev/ttyS10", argv[1]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[1]) != 0)))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	/*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0){
		perror(argv[1]);
		exit(-1);
	}

	if (tcgetattr(fd, &oldtio) == -1){ /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 1;	/* blocking read until 5 chars received */

	/* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");


	char set[255];

	// Receive SET
	readPort(fd, set);

	printf("SET received: %s:%d\n",set, strlen(set) + 1);

	// Send UA
	res = write(fd, set, strlen(set) + 1);
	printf("%d bytes written\n", res);

	sleep(1);
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
	return 0;
}
