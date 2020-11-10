#ifndef READ_H
#define READ_H

#include "state_machine.h"

int read_frame_reader(int fd, char** data, frame_type frame_type);

void read_set(int fd);
void read_disc(int fd);
int read_info(int fd, char** buffer);

int send_disc_receiver(int fd);
void disc_alarm_receiver();
void sigalarm_disc_handler_reader(int sig);

#endif