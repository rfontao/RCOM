#ifndef READ_H
#define READ_H

int read_frame(int fd, unsigned char *data);

void read_set(int fd);

int send_disc_receiver(int fd);

void read_disc(int fd);

void sigalarm_disc_handler(int sig);

#endif