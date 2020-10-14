#ifndef _COMMON_H
#define _COMMON_H

typedef enum  {
    UA,
    SET,
    DISC,
    RR
} frameType;

void write_to_port(int fd, char* data, size_t s);

void send_frame(int fd, frameType type);

#endif