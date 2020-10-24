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

static volatile int STOP = FALSE;

#define MAX_FRAME_TRIES 3
#define FRAME_TIMEOUT 3


static int disc_tries = 0;
static int alarm_flag = 0;
static int retry_flag = 0;

unsigned int timeout;
unsigned int numTransmissions;

int read_frame_reader(int fd, char** data, frame_type frame_type){
	*data = (char*)malloc(sizeof(char) * INITIAL_ALLOC_SIZE);
	long current_alloc_size = INITIAL_ALLOC_SIZE;
	char result;
	STATE st;
	int stuffed = 0;
	int i = 0;
	unsigned char c;

	printf("Trying to read frame:\n");

	while(STOP == FALSE) {

		if(i == current_alloc_size - 2){
			*data = (char*)realloc(*data, current_alloc_size * 2);
			current_alloc_size *= 2;
		}
		read(fd, &result, 1);
		if(retry_flag == 1){
			return -1;
		}

		if(result == ESC) {
			stuffed = 1;
			continue;
		}
		if(stuffed) {
			if(result == SEQ_1)
				(*data)[i] = FLAG;
			else if(result == SEQ_2)
				(*data)[i] = ESC;
			else {
				send_rej(fd, c);
				printf("REJ\n");
			}
			stuffed = 0;
		} else 
			(*data)[i] = result;
		i++;

		st = machine(result, RECEIVER_M, frame_type);
		if(st == STOP_ST || st == STOP_INFO) {
			STOP = TRUE;
		} else if(st == C_RCV) {
			c = result;
		}
	}
	STOP = FALSE;

	if(st == STOP_ST){
		// for(int k = 0; k < 5; ++k){
		// 	data[k] = buf[k];
		// }

		return 5; //TODO: Maybe change for the size of normal frame
	}

	printf("RECEIVED INFO %s : %d\n", *data, i);


	int j = 5;
	unsigned char bcc2_check = (*data)[j-1];

	for(; j < i - 2; ++j) 
        bcc2_check ^= (*data)[j];

	if((unsigned char)((*data)[i - 2]) != bcc2_check) {
		send_rej(fd, c);
	} else {
		send_rr(fd, c);
	}

	int k = 4;
	for(; k < i - 2; ++k){
		(*data)[k - 4] = (*data)[k];
	}

	// print_frame(*data, i);

	return k - 4;
}

void read_set(int fd){
	char* frame;

	while(1){
		read_frame_reader(fd, &frame, COMMAND);
		if(frame[2] == C_SET){
			printf("Received SET frame\n");
			send_frame(fd, UA_RECEIVER);
			break;
		}
	}
	free(frame);
}

void read_disc(int fd){
	char* frame;

	while(1){
		read_frame_reader(fd, &frame, COMMAND);
		if(frame[2] == C_DISC){
			printf("Received DISC frame\n");
			break;
		}
	}
	free(frame);
}

int read_info(int fd, char** buffer){
	int read_size;
	if((read_size = read_frame_reader(fd, buffer, COMMAND)) < 0){
		return -1;
	}
	//if(buffer[4] == FLAG){
	//	printf("Read strange packet. Expected info\n");
	//	return -1;
	//}
	return read_size;
}

void disc_alarm_receiver(){
	struct sigaction a;
	a.sa_handler = sigalarm_disc_handler_reader;
	a.sa_flags = 0;
	sigemptyset(&a.sa_mask);
	sigaction(SIGALRM, &a, NULL);
	printf("DISC Alarm handler set\n");
}

int send_disc_receiver(int fd){

    disc_alarm_receiver();

	alarm(FRAME_TIMEOUT);
    send_frame(fd, DISC_RECEIVER);

	char* ua_frame;

	while(alarm_flag == 0){
		read_frame_reader(fd, &ua_frame, RESPONSE);
		if(retry_flag == 1){
			send_frame(fd, DISC_RECEIVER);
			retry_flag = 0;
			continue;
		}
		if(ua_frame[2] == C_UA){
			printf("Received UA\n");
			alarm(0);
			free(ua_frame);
			return 0;
		} else {
			printf("Wasn't UA\n");
		}
	}
	alarm_flag = 0;
	free(ua_frame);
	return -1;
}

void sigalarm_disc_handler_reader(int sig){
    if (disc_tries < MAX_FRAME_TRIES){
        printf("Alarm timeout\n");
        disc_tries++;
		retry_flag = 1;
        // send_frame(fd, DISC_RECEIVER);
        alarm(FRAME_TIMEOUT);
    } else {
        perror("DISC max tries reached\n");
        alarm_flag = 1;
    }
}
