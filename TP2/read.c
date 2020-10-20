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
#include "read.h"

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

int read_frame(int fd, unsigned char *data){
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

void read_set(int fd){
	unsigned char frame[255];

	while(1){
		read_frame(fd, frame);
		if(frame[2] == C_SET){
			printf("Received SET frame\n");
			send_frame(fd, UA_RECEIVER);
			break;
		}
	}
}

int send_disc_receiver(int fd){
	unsigned char disc_frame[255];

    (void)signal(SIGALRM, sigalarm_disc_handler);
    printf("DISC Alarm handler set\n");

	alarm(FRAME_TIMEOUT);
    send_frame(fd, DISC_SENDER);

	unsigned char ua_frame[255];

	while(alarm_flag == 0){
		read_frame(fd, ua_frame);
		if(disc_frame[2] == C_UA){
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

void read_disc(int fd){
	unsigned char frame[255];

	while(1){
		read_frame(fd, frame);
		if(frame[2] == C_DISC){
			printf("Received DISC frame\n");
			break;
		}
	}
}

void sigalarm_disc_handler(int sig){
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
