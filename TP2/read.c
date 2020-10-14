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
#include "common.h"
#include "state_machine.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

#define MAX_FRAME_TRIES 3
#define FRAME_TIMEOUT 3

int fd;
int disc_tries = 0;
int alarm_flag = 0;

void read_ua_frame(int fd, char *out){

    printf("Trying to read UA\n");

    char buf[255];

    char result;
	STATE st;

    while (STOP == FALSE){
		read(fd, &result, 1);
		if((st = ua_receiver_machine(result)) > 0)
			buf[st - 1] = result;

		if(st == STOP_ST)
			STOP = TRUE;
	}

    *out = *buf;
	STOP = FALSE;

    printf("UA received : %s : %d bytes\n", buf, st);
}

void read_set_frame(int fd, char* frame){

	printf("Trying to read SET\n");

	char buf[255];

	char result;
	STATE st;

	while (STOP == FALSE){
		read(fd, &result, 1);
		if((st = set_machine(result)) > 0)
			buf[st - 1] = result;

		if(st == STOP_ST) {
			STOP = TRUE;
			alarm(0);
		}
	}

	printf("SET received: %s : %d\n", buf, st);
	STOP = FALSE;

	*frame = *buf;
}

void read_disc_frame(int fd, char *out){

    printf("Trying to read DISC\n");
    alarm(FRAME_TIMEOUT);

    char buf[255];

    char result;
	STATE st;

    while (STOP == FALSE){
        read(fd, &result, 1);
        if (alarm_flag == 1){
            printf("AAAAAAAAAAAAAAAAAAAAA\n");
            alarm_flag = 0;
            continue;
        }
		if((st = disc_receiver_machine(result)) > 0)
        	buf[st - 1] = result;

		if(st == STOP_ST) {
			STOP = TRUE;
			alarm(0);
		}
    }

    *out = *buf;
	STOP = FALSE;

    printf("DISC received : %s : %d bytes\n", buf, st);
}

void sigalarm_disc_handler(int sig){
    if (disc_tries < MAX_FRAME_TRIES){
        printf("Alarm timeout\n");
        disc_tries++;
        alarm_flag = 1;
        send_frame(fd, DISC_SENDER);
        alarm(FRAME_TIMEOUT);
    } else {
        perror("DISC max tries reached exiting...\n");
        exit(2);
    }
}

int main(int argc, char **argv){
	struct termios oldtio, newtio;

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

	char set_frame[255];

	// Receive SET
	read_set_frame(fd, set_frame);

	//printf("AAA: %x %c\n", set_frame[0], FLAG);

	if(set_frame[0] == FLAG){
		send_frame(fd, UA_RECEIVER);
	}

    (void)signal(SIGALRM, sigalarm_disc_handler);
    printf("DISC Alarm handler set\n");

    char disc_frame[255];

    read_disc_frame(fd, disc_frame);
    send_frame(fd, DISC_RECEIVER);

    char ua_frame[255];

    read_ua_frame(fd, ua_frame);
    

    sleep(1);
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
	return 0;
}
