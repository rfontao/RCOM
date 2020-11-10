#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "message_macros.h"


void write_to_port(int fd, char* data, size_t s){
	int sent = write(fd, data, s);
	//printf("%d bytes written\n", sent);
}

void print_frame(char* frame, size_t s){
    for(int i = 0; i < s; i++){
            printf(" %x ", (unsigned char)(frame[i]));
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
        printf("Sent SET frame");
        //print_frame(set_frame, 5);
    }

    if(type == UA_RECEIVER){
        char ua_frame[5];
        ua_frame[0] = FLAG;
        ua_frame[1] = A_SENDER;
        ua_frame[2] = C_UA;
        ua_frame[3] = A_SENDER ^ C_UA;
        ua_frame[4] = FLAG;

	    write_to_port(fd, ua_frame, 5);
        printf("Sent UA frame");
        //print_frame(ua_frame, 5);
    }

    if (type == UA_SENDER){
        char ua_frame[5];
        ua_frame[0] = FLAG;
        ua_frame[1] = A_RECEIVER;
        ua_frame[2] = C_UA;
        ua_frame[3] = A_RECEIVER ^ C_UA;
        ua_frame[4] = FLAG;

        write_to_port(fd, ua_frame, 5);
        printf("Sent UA frame");
        //print_frame(ua_frame, 5);
    }

    if(type == DISC_RECEIVER){
        char disc_frame[5];
        disc_frame[0] = FLAG;
        disc_frame[1] = A_RECEIVER;
        disc_frame[2] = C_DISC;
        disc_frame[3] = A_RECEIVER ^ C_DISC;
        disc_frame[4] = FLAG;

        write_to_port(fd, disc_frame, 5);
        printf("Sent DISC frame");
        //print_frame(disc_frame, 5);
    }

    if (type == DISC_SENDER){
        char disc_frame[5];
        disc_frame[0] = FLAG;
        disc_frame[1] = A_SENDER;
        disc_frame[2] = C_DISC;
        disc_frame[3] = A_SENDER ^ C_DISC;
        disc_frame[4] = FLAG;

        write_to_port(fd, disc_frame, 5);
        printf("Sent DISC frame");
        //print_frame(disc_frame, 5);
    }
}

unsigned char calculate_bcc2(char* data, size_t size) {
    unsigned char result = data[0];

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
    rr[1] = A_SENDER;
    
    if(c == C_INFO1)
        rr[2] = C_RR_1;
    else if(c == C_INFO2)
        rr[2] = C_RR_2;

    rr[3] = A_SENDER ^ rr[2];
    rr[4] = FLAG;

    printf("Sent RR");
    //print_frame(rr, 5);

    write_to_port(fd, rr, 5);
}

void send_rej(int fd, char c) {
    char rej[5];
    rej[0] = FLAG;
    rej[1] = A_SENDER;
    
    if(c == C_INFO1)
        rej[2] = C_REJ_1;
    else if(c == C_INFO2)
        rej[2] = C_REJ_2;

    rej[3] = A_SENDER ^ rej[2];
    rej[4] = FLAG;

    printf("Sent REJ");
    //print_frame(rej, 5);

    write_to_port(fd, rej, 5);
}