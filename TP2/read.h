#ifndef READ_H
#define READ_H

#include "state_machine.h"

int read_frame_reader(int fd, unsigned char *data, frame_type frame_type);

void read_set(int fd);

int send_disc_receiver(int fd);

void read_disc(int fd);

void sigalarm_disc_handler_reader(int sig);

int read_info(int fd, char* buffer);

#endif