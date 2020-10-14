#ifndef _COMMON_H
#define _COMMON_H

enum frameType {
    UA_SENDER,
    UA_RECEIVER,
    SET,
    DISC_SENDER,
    DISC_RECEIVER,
    RR
};

void write_to_port(int fd, char* data, size_t s);

void send_frame(int fd, enum frameType type);

#endif