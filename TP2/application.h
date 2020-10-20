#ifndef _APPLICATION_H
#define _APPLICATION_H

#define SENDER      0
#define RECEIVER    1

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

#define START 0
#define END 1

#define C_DATA  0x01
#define C_START 0x02
#define C_END   0x03

#define FILE_SIZE    0
#define FILE_NAME    1

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

int assemble_control_packet(int type, char *filename, int fileSize, unsigned char* packet);

int assemble_data_packet(unsigned char* data, int length, unsigned char* packet);

int send_control(int type, char *filename, int fileSize);

#endif