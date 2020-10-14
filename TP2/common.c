#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "message_macros.h"


void write_to_port(int fd, char* data, size_t s){
	int sent = write(fd, data, s);
	printf("%d bytes written\n", sent);
}

void print_frame(char* frame, size_t s){
    for(int i = 0; i < s; i++){
            printf(" %d ",frame[i]);
        }
    printf("\n");
}

void send_frame(int fd, frameType type){

    if(type == SET){
        char set_frame[5];
	    set_frame[0] = FLAG;
        set_frame[1] = A_SENDER;
        set_frame[2] = C_SET;
        set_frame[3] = A_SENDER ^ C_SET;
        set_frame[4] = FLAG;

        write_to_port(fd, set_frame, 5);
        printf("Sent SET frame : ");
        print_frame(set_frame, 5);
    }

    if(type == UA_RECEIVER){
        char ua_frame[5];
        ua_frame[0] = FLAG;
        ua_frame[1] = A_RECEIVER;
        ua_frame[2] = C_UA;
        ua_frame[3] = A_RECEIVER ^ C_UA;
        ua_frame[4] = FLAG;

	    write_to_port(fd, ua_frame, 5);
        printf("Sent UA frame : ");
        print_frame(ua_frame, 5);
    }

    if (type == UA_SENDER){
        char ua_frame[5];
        ua_frame[0] = FLAG;
        ua_frame[1] = A_SENDER;
        ua_frame[2] = C_UA;
        ua_frame[3] = A_SENDER ^ C_UA;
        ua_frame[4] = FLAG;

        write_to_port(fd, ua_frame, 5);
        printf("Sent UA frame : ");
        print_frame(ua_frame, 5);
    }

    if(type == DISC_RECEIVER){
        char disc_frame[5];
        disc_frame[0] = FLAG;
        disc_frame[1] = A_RECEIVER;
        disc_frame[2] = C_DISC;
        disc_frame[3] = A_RECEIVER ^ C_DISC;
        disc_frame[4] = FLAG;

        write_to_port(fd, disc_frame, 5);
        print_frame(disc_frame, 5);
        printf("Sent DISC frame : ");
        print_frame(disc_frame, 5);
    }

    if (type == DISC_SENDER){
        char disc_frame[5];
        disc_frame[0] = FLAG;
        disc_frame[1] = A_SENDER;
        disc_frame[2] = C_DISC;
        disc_frame[3] = A_SENDER ^ C_DISC;
        disc_frame[4] = FLAG;

        write_to_port(fd, disc_frame, 5);
        printf("Sent DISC frame : ");
        print_frame(disc_frame, 5);
    }

    
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
}

char calculate_bcc2(char* data, size_t size) {
    char result = data[0];

    for(int i = 1; i < size; ++i) 
        result ^= data[i];

    return result;
}

int stuff_data(char* data, size_t size, char* stuffed) {
    int i = 1, j = 1;
    stuffed[0] = data[0];

    for(; i < size - 1; ++i) {

        if(data[i] == FLAG) {
            stuffed[j] = ESC;
            stuffed[++j] = SEQ_1;
        } else if(data[i] == ESC) {
            stuffed[j] = ESC;
            stuffed[++j] = SEQ_2;
        } else {
            stuffed[j] = data[i];
        }
        j++;
    }
    stuffed[j] = data[size - 1];

    return j+1;
}

void send_rr(int fd, char c) {
    char rr[5];
    rr[0] = FLAG;
    rr[1] = A_RECEIVER;
    
    if(c == C_INFO1)
        rr[2] = C_RR_1;
    else if(c == C_INFO2)
        rr[2] = C_RR_2;

    rr[3] = A_RECEIVER ^ rr[2];
    rr[4] = FLAG;

    write_to_port(fd, rr, 5);
}

void send_rej(int fd, char c) {
    char rr[5];
    rr[0] = FLAG;
    rr[1] = A_RECEIVER;
    
    if(c == C_INFO1)
        rr[2] = C_REJ_1;
    else if(c == C_INFO2)
        rr[2] = C_REJ_2;

    rr[3] = A_RECEIVER ^ rr[2];
    rr[4] = FLAG;

    write_to_port(fd, rr, 5);
}