/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 			"01111110"
#define SENDER_A 		"00000011"
#define SENDER_C_SET 	"00000011"
#define BCC1 			"00000000"

volatile int STOP = FALSE;

void writeToPort(int fd, void* data){
	int sent = write(fd, data, strlen(data) + 1);
	printf("%d bytes written\n", sent);
}


int main(int argc, char **argv){
	int fd, res;
	struct termios oldtio, newtio;
	char buf[255];

	if ((argc < 2) ||
		((strcmp("/dev/ttyS10", argv[1]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[1]) != 0)))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
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

	/* SET asswembly */
	/* [F,A,C,BCC1,F] */
	strcpy(buf, FLAG);
	strcat(buf, SENDER_A);
	strcat(buf, SENDER_C_SET);
	strcat(buf, BCC1);
	strcat(buf, FLAG);



	// fgets(buf,255,stdin);
	// //Put \0 on the end of the read string
	// buf[strlen(buf) - 1] = '\0';

	//Sending the data
	writeToPort(fd, buf);


	char* result = (char*)malloc(sizeof(char));
	int a = 0;

	while (STOP == FALSE){
		res = read(fd, result, 1);
		if(a == 0)
			strcpy(buf, result);
		else
			strcat(buf, result);
		a++;
		if (result[0] == '\0')
			STOP = TRUE;
	}

	printf(":%s:%d\n", buf, a);

	sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}


