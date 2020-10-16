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
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_FRAME_TRIES 	3
#define FRAME_TIMEOUT		3

volatile int STOP = FALSE;

int fd; 	/*TODO: Change later*/
int set_tries = 0;
int disc_tries = 0;
int alarm_flag = 0;
int info_tries = 0;

char lastSent[1024];
size_t last_sent_size = 0;

void read_frame(int fd, char* out){

	printf("Trying to read frame:\n");
	//alarm(FRAME_TIMEOUT);

	char result;
	STATE st;

	while (STOP == FALSE){
		read(fd, &result, 1);
		if((st = machine(result, SENDER)) > 0)
			out[st - 1] = result;
		
		if(st == STOP_ST)
			STOP = TRUE;
	}
	STOP = FALSE;

	printf("Received frame: ");
}

void send_info_frame(int fd, char* data, size_t size, int resend) {

    static int c = 0;

	if(resend == TRUE){
		write_to_port(fd, lastSent, last_sent_size);
		return;
	}

    char frame[1024];

    frame[0] = FLAG;
    frame[1] = A_SENDER;

    if(c % 2 == 0)
        frame[2] = C_INFO1;
    else frame[2] = C_INFO2;

    frame[3] = A_SENDER ^ frame[2];

    int i = 0;
    for(; i < size; ++i) {
        frame[i + 4] = data[i];
    }

    frame[i + 4] = calculate_bcc2(data, size);
    frame[i + 5] = FLAG;

    char stuffed_frame[1024];

    int frame_size = stuff_data(frame, i + 6, stuffed_frame);

    c++;

	alarm(FRAME_TIMEOUT);

	for(int k = 0; k < frame_size; k++){
		lastSent[i] = stuffed_frame[i];
	}
	last_sent_size = frame_size;

    write_to_port(fd, stuffed_frame, frame_size);
    printf("Sent INFO frame : %s : %d\n", stuffed_frame, frame_size);
}

void sigalarm_set_handler(int sig){
	if(set_tries < MAX_FRAME_TRIES){
		printf("Alarm timeout\n");
		set_tries++;
		alarm_flag = 1;
		send_frame(fd, SET);
		alarm(FRAME_TIMEOUT);
	} else {
		perror("SET max tries reached exiting...\n");
		exit(2);
	}
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

void sigalarm_info_handler(int sig){
    if (info_tries < MAX_FRAME_TRIES){
        printf("Alarm timeout\n");
        info_tries++;
        alarm_flag = 1;
        send_info_frame(fd, lastSent, last_sent_size, TRUE);
        alarm(FRAME_TIMEOUT);
    } else {
        perror("INFO max tries reached exiting...\n");
        exit(2);
    }
}

int main(int argc, char **argv){
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
	
	//Takes care of resending SET frames in case of timeout
	(void) signal(SIGALRM, sigalarm_set_handler);
	printf("SET Alarm handler set\n");

	char ua_frame[255];
	//First send
	send_frame(fd, SET);

	while(1){
		read_frame(fd, ua_frame);
		if(ua_frame[2] == C_UA){
			printf("Received UA\n");
			printf("---Connection established---\n");
			alarm(0);
			break;
		} else {
			printf("Wasn't UA\n");
		}
	}


	(void)signal(SIGALRM, sigalarm_info_handler);
    printf("INFO Alarm handler set\n");


	char response[255];
	int resend = FALSE;

	char data[] = "Hello world!";

	while(1){
		send_info_frame(fd, data, 13, resend);
		read_frame(fd, response);

		if(response[2] == C_RR_1 || response[2] == C_RR_2) {
			printf("RR received\n");
			resend = FALSE;
			alarm(0);
			break; // for now only a string of data
		} else if(response[2] == C_REJ_1 || response[2] == C_REJ_2) {
			printf("REJ received\n");
			resend = TRUE;
		}
	}


    char disc_frame[255];
    (void)signal(SIGALRM, sigalarm_disc_handler);
    printf("DISC Alarm handler set\n");

    send_frame(fd, DISC_SENDER);
    read_frame(fd, disc_frame);
	if(disc_frame[2] == C_DISC){
		alarm(0);
    	send_frame(fd, UA_SENDER);
	}
	

    sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}


