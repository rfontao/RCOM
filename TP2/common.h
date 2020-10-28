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

#define MAX_CHUNK_SIZE 400
#define MAX_PACKET_SIZE MAX_CHUNK_SIZE + 4
#define MAX_FRAME_SIZE MAX_PACKET_SIZE + 6

void write_to_port(int fd, char* data, size_t s);
void send_frame(int fd, frameType type);

void print_frame(char* frame, size_t s);

unsigned char calculate_bcc2(char* data, size_t size);
int stuff_data(char* data, size_t size, char* stuffed);

void send_rr(int fd, char c);
void send_rej(int fd, char c);

#endif