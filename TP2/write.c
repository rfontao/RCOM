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
#include "write.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_FRAME_TRIES 	3
#define FRAME_TIMEOUT		3

static volatile int STOP = FALSE;

static int fd; 	/*TODO: Change later*/
static int set_tries = 0;
static int disc_tries = 0;
static int alarm_flag = 0;
static int info_tries = 0;

unsigned char lastSent[1024];
size_t last_sent_size = 0;

void read_frame_writer(int fd, unsigned char* out, frame_type frame_type){

	printf("Trying to read frame:\n");
	//alarm(FRAME_TIMEOUT);

	unsigned char result;
	STATE st;

	while (STOP == FALSE){
		read(fd, &result, 1);
		if(alarm_flag == 1){
			//TODO: Dar reset ao estado?
			alarm_flag = 0;
			reset_state(SENDER_M);
			continue;
		}
		if((st = machine(result, SENDER_M, frame_type)) > 0)
			out[st - 1] = result;
		
		if(st == STOP_ST)
			STOP = TRUE;
	}
	STOP = FALSE;

	printf("Received frame: ");
}

void send_info_frame(int fd, unsigned char* data, size_t size, int resend) {

    static int c = 0;

	if(resend == TRUE){
		write_to_port(fd, lastSent, last_sent_size);
		return;
	}

    unsigned char frame[1024];

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

    unsigned char stuffed_frame[1024];

    int frame_size = stuff_data(frame, i + 6, stuffed_frame);

    c++;

	for(int k = 0; k < frame_size; k++){
		lastSent[i] = stuffed_frame[i];
	}
	last_sent_size = frame_size;

    write_to_port(fd, stuffed_frame, frame_size);
    printf("Sent INFO frame : %s : %d\n", stuffed_frame, frame_size);
}

void sigalarm_set_handler_writer(int sig){
	if(set_tries < MAX_FRAME_TRIES){
		printf("Alarm timeout\n");
		set_tries++;
		send_frame(fd, SET);
		alarm(FRAME_TIMEOUT);
	} else {
		perror("SET max tries reached\n");
		alarm_flag = 1;
	}
}

void sigalarm_disc_handler_writer(int sig){
    if (disc_tries < MAX_FRAME_TRIES){
        printf("Alarm timeout\n");
        disc_tries++;
        send_frame(fd, DISC_SENDER);
        alarm(FRAME_TIMEOUT);
    } else {
        perror("DISC max tries reached\n");
        alarm_flag = 1;
    }
}

void sigalarm_info_handler_writer(int sig){
    if (info_tries < MAX_FRAME_TRIES){
        printf("Alarm timeout\n");
        info_tries++;
        send_info_frame(fd, lastSent, last_sent_size, TRUE);
        alarm(FRAME_TIMEOUT);
    } else {
        perror("INFO max tries reached\n");
        alarm_flag = 1;
    }
}

int send_set(int fd){
	(void) signal(SIGALRM, sigalarm_set_handler_writer);
	printf("SET Alarm handler set\n");

	alarm(FRAME_TIMEOUT);

	unsigned char ua_frame[255];
	//First send
	send_frame(fd, SET);

	while(alarm_flag == 0){
		read_frame_writer(fd, ua_frame, RESPONSE);
		if(ua_frame[2] == C_UA){
			printf("Received UA\n");
			alarm(0);
			return 0;
		} else {
			printf("Wasn't UA\n");
		}
	}
	alarm_flag = 0;
	return -1;
}

int send_disc_sender(int fd){
	unsigned char disc_frame[255];

    (void)signal(SIGALRM, sigalarm_disc_handler_writer);
    printf("DISC Alarm handler set\n");

	alarm(FRAME_TIMEOUT);
    send_frame(fd, DISC_SENDER);

	while(alarm_flag == 0){
		read_frame_writer(fd, disc_frame, COMMAND);
		if(disc_frame[2] == C_DISC){
			printf("Received DISC\n");
			alarm(0);
			send_frame(fd, UA_SENDER);
			return 0;
		} else {
			printf("Wasn't DISC\n");
		}
	}
	alarm_flag = 0;
	return -1;
}

int send_info(int fd, char* data, int length){

	(void)signal(SIGALRM, sigalarm_info_handler_writer);
    printf("INFO Alarm handler set\n");

	alarm(FRAME_TIMEOUT);

	unsigned char response[255];
	int resend = FALSE;
	while(alarm_flag == 0){
		send_info_frame(fd, data, length, resend);
		read_frame_writer(fd, response, RESPONSE);

		print_frame(response, 5);

		if(response[2] == C_RR_1 || response[2] == C_RR_2) {
			printf("RR received\n");
			resend = FALSE;
			alarm(0);
			return length;
		} else if(response[2] == C_REJ_1 || response[2] == C_REJ_2) {
			printf("REJ received\n");
			resend = TRUE;
		}
	}

	alarm_flag = 1;
	return -1;
}
