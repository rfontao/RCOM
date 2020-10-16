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

size_t read_frame(int fd, unsigned char *data){
	unsigned char buf[1024];
	unsigned char result;
	STATE st;
	int stuffed = 0;
	int i = 0;
	unsigned char c;

	printf("Trying to read frame:\n");

	while(STOP == FALSE) {
		read(fd, &result, 1);

		if(alarm_flag == 1){
			alarm_flag = 0;
			reset_state(RECEIVER);
			continue;
		}
		if(result == ESC) {
			stuffed = 1;
			continue;
		}
		if(stuffed) {
			if(result == SEQ_1)
				buf[i] = FLAG;
			else if(result == SEQ_2)
				buf[i] = ESC;
			else {
				send_rej(fd, c);
				printf("REJ\n");
			}
			stuffed = 0;
		} else 
			buf[i] = result;
		i++;

		st = machine(result, RECEIVER);
		if(st == STOP_ST || st == STOP_INFO) {
			STOP = TRUE;
		} else if(st == C_RCV) {
			c = result;
		}
	}
	STOP = FALSE;

	if(st == STOP_ST){
		for(int k = 0; k < 5; ++k){
			data[k] = buf[k];
		}

		return 0; //TODO: Maybe change for the size of normal frame
	}

	int j = 5;
	unsigned char bcc2_check = buf[j-1];

	for(; j < i - 2; ++j) 
        bcc2_check ^= buf[j];

	if(buf[i - 2] != bcc2_check) {
		send_rej(fd, c);
	} else {
		send_rr(fd, c);
	}

	int k = 4;
	for(; k < i - 2; ++k){
		data[k - 4] = buf[k];
	}

	return k - 4;
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

	unsigned char frame[255];
	// Receive SET
	while(1){
		read_frame(fd, frame);
		if(frame[2] == C_SET){
			printf("Received SET frame\n");
			send_frame(fd, UA_RECEIVER);
			break;
		}
	}

	unsigned char data[4096];
	unsigned char buffer[1024];
	size_t buffer_count = 0;
	while(1){
		size_t read_size = read_frame(fd, buffer);
		if(buffer[2] == C_DISC){
			printf("Received DISC frame\n");
			break;
		} else {
			for(size_t i = 0; i < read_size; ++i){
				data[buffer_count + i] = buffer[i];
			}
			buffer_count += read_size;
		}
	}
	write(STDOUT_FILENO, data, buffer_count);
	printf("\n");

    (void)signal(SIGALRM, sigalarm_disc_handler);
    printf("DISC Alarm handler set\n");

	alarm(FRAME_TIMEOUT);
    send_frame(fd, DISC_RECEIVER);


	//TODO: make send disc receiver frame again if it does not get response
    unsigned char ua_frame[255];
    read_frame(fd, ua_frame);

	if(ua_frame[2] == C_UA){
		printf("Received UA\n");
		alarm(0);
	}
    
    sleep(1);
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
	return 0;
}
