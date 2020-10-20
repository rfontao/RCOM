#ifndef _APPLICATION_H
#define _APPLICATION_H

#define SENDER      0
#define RECEIVER    1

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

typedef struct 
{
    int fileDescriptor;
    int status; /*SENDER | RECEIVER*/
} applicationLayer;


int llopen(char* port, int mode);

int set_port(char* port);

int llwrite(int fd, char* buffer, int length);

int llread(int fd, char* buffer);

int llclose(int fd);

int close_port();

#endif