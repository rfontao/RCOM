#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "message_macros.h"


void write_to_port(int fd, char* data, size_t s){
	int sent = write(fd, data, s);
	printf("%d bytes written\n", sent);
}

void send_frame(int fd, enum frameType type){

    if(type == SET){
        char set_frame[5];
	    set_frame[0] = FLAG;
        set_frame[1] = A_SENDER;
        set_frame[2] = C_SET;
        set_frame[3] = A_SENDER ^ C_SET;
        set_frame[4] = FLAG;

        write_to_port(fd, set_frame, 5);
        printf("Sent SET frame\n");
    }

    if(type == UA){
        char ua_frame[5];
        ua_frame[0] = FLAG;
        ua_frame[1] = A_RECEIVER;
        ua_frame[2] = C_UA;
        ua_frame[3] = A_RECEIVER ^ C_UA;
        ua_frame[4] = FLAG;

	    write_to_port(fd, ua_frame, 5);
        printf("Sent UA frame\n");
    }
}