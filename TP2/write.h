#ifndef WRITE_H
#define WRITE_H

void read_frame(int fd, unsigned char* out);

void send_info_frame(int fd, unsigned char* data, size_t size, int resend);

void sigalarm_set_handler(int sig);

void sigalarm_disc_handler(int sig);

void sigalarm_info_handler(int sig);

int send_set(int fd);

int send_disc_sender(int fd);

int send_info(int fd, char* data, int length);

#endif