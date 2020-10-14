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

char lastSent[255];

void read_ua_frame(int fd, char* out){

	printf("Trying to read UA\n");
	alarm(FRAME_TIMEOUT);

	char buf[255];

	char result;
	STATE st;

	while (STOP == FALSE){
		read(fd, &result, 1);
		if(alarm_flag == 1){
			printf("AAAAAAAAAAAAAAAAAAAAA\n");
			alarm_flag = 0;
			continue;
		}
		if((st = ua_sender_machine(result)) > 0)
			buf[st - 1] = result;
		
		if(st == STOP_ST) {
			STOP = TRUE;
			alarm(0);
		}
	}

	*out = *buf;
	STOP = FALSE;

	printf("UA received: ");
    print_frame(buf, 5);
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
		if((st = disc_sender_machine(result)) > 0)
        	buf[st - 1] = result;

        if(st == STOP_ST) {
			STOP = TRUE;
			alarm(0);
		}
    }

    *out = *buf;
	STOP = FALSE;

    printf("DISC received: ");
    print_frame(buf, 5);
}

void read_info_response(int fd) {
	char buf[255];
	char result;
	char c;
	STATE st;

	while(STOP == FALSE) {
		read(fd, &result, 1);
		if((st = info_response_machine(result)) == STOP_ST) {
			STOP = TRUE;
		} else if(st == C_RCV) 
			c = result;
		if(st > 0)
			buf[st - 1] = result;
	}
	STOP = FALSE;

	if(c == C_RR_1 || c == C_RR_2) {
		printf("RR received: ");
	} else if(c == C_REJ_1 || c == C_REJ_2) {
		printf("REJ received: ");
	}
	print_frame(buf, 5);

}

void send_info_frame(int fd, char* data, size_t size, int resend) {

    char frame[255];

    static int c = 0;
    c += resend;

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

    char stuffed_frame[255];

    int frame_size = stuff_data(frame, i + 6, stuffed_frame);

    c++;

    write_to_port(fd, stuffed_frame, frame_size);
    printf("Sent INFO frame : %s : %d\n", stuffed_frame, frame_size);

    read_info_response(fd);
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
	
	(void) signal(SIGALRM, sigalarm_set_handler);
	printf("SET Alarm handler set\n");

	char ua_frame[255];

	send_frame(fd, SET);
	read_ua_frame(fd, ua_frame);

    (void)signal(SIGALRM, sigalarm_disc_handler);
    printf("DISC Alarm handler set\n");

    char disc_frame[255];

    send_frame(fd, DISC_SENDER);
    read_disc_frame(fd, disc_frame);
    send_frame(fd, UA_SENDER);

    sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}


