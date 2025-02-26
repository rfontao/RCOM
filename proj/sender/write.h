#ifndef WRITE_H
#define WRITE_H

#include <stdlib.h>
#include "state_machine.h"

void read_frame_writer(int fd, char* out, frame_type frame_type);
void send_info_frame(int fd, char* data, size_t size, int resend);

void sigalarm_set_handler_writer(int sig);
void sigalarm_disc_handler_writer(int sig);
void sigalarm_info_handler_writer(int sig);
void set_alarm();
void info_alarm();
void disc_alarm_writer();

int send_set(int fd);
int send_disc_sender(int fd);
int send_info(int fd, char* data, int length);

#endif