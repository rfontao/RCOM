/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "message_macros.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_SET_TRIES 	3
#define SET_TIMEOUT		3

volatile int STOP = FALSE;

int fd; 	/*TODO: Change later*/
int set_tries = 0;
int ua_frame[255];

void write_to_port(int fd, char* data, size_t s){
	int sent = write(fd, data, s);
	printf("%d bytes written\n", sent);
}

void send_set_frame(int fd){
	char set_frame[5];
	set_frame[0] = FLAG;
	set_frame[1] = A_SENDER;
	set_frame[2] = C_SET;
	set_frame[3] = A_SENDER ^ C_SET;
	set_frame[4] = FLAG;

	write_to_port(fd, set_frame, 5);
}

void read_ua_frame(int fd){

	printf("Trying to read UA\n");
	alarm(SET_TIMEOUT);

	int flag_count = 0;
	char buf[255];

	char result;
	int i = 0;

	while (STOP == FALSE){
		read(fd, &result, 1);
		ua_frame[i] = result;
		i++;
		if (result == FLAG){
			if(flag_count > 0) {
				STOP = TRUE;
				alarm(0);
			} else {
				flag_count++;
			}
		}
	}

	printf(":%s:%d\n", buf, i);
}

void sigalarm_handler(int sig){
	if(set_tries < MAX_SET_TRIES){
		printf("%d\n",set_tries);
		set_tries++;
		printf("Alarm timeout\n");
		send_set_frame(fd);
		read_ua_frame(fd);
	} else {
		perror("SET max tries reached exiting...\n");
		exit(2);
	}
}

int main(int argc, char **argv){
	int res;
	struct termios oldtio, newtio;
	

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
	
	(void) signal(SIGALRM, sigalarm_handler);
	printf("Alarm handler set\n");

	send_set_frame(fd);
	read_ua_frame(fd);

	sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}


