#ifndef _COMMON_H
#define _COMMON_H

typedef enum {
    UA_SENDER,
    UA_RECEIVER,
    SET,
    DISC_SENDER,
    DISC_RECEIVER,
    RR,
    REJ
} frameType;

void write_to_port(int fd, unsigned char* data, size_t s);

void print_frame(unsigned char* frame, size_t s);

void send_frame(int fd, frameType type);

unsigned char calculate_bcc2(unsigned char* data, size_t size);

int stuff_data(unsigned char* data, size_t size, unsigned char* stuffed);

void send_rr(int fd, unsigned char c);

void send_rej(int fd, unsigned char c);

#endif