#ifndef _APPLICATION_H
#define _APPLICATION_H

#define SENDER      0
#define RECEIVER    1

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

#define START_C 0
#define END_C 1

#define C_DATA  0x01
#define C_START 0x02
#define C_END   0x03

#define FILE_SIZE    0
#define FILE_NAME    1

typedef struct 
{
    int fileDescriptor;
    int status; /*SENDER | RECEIVER*/
    FILE* file;
} applicationLayer;

int set_port(char* port);
int close_port();

int llopen(char* port, int mode);
int llwrite(int fd, char* buffer, int length);
int llread(int fd, char* buffer);
int llclose(int fd);

int assemble_control_packet(int type, char *filename, long fileSize, char* packet);
int assemble_data_packet(char* data, int length, int sequenceN, char* packet);

int send_control(int type, char *filename, long fileSize);
int read_control(char* ctl, char* fileName, int* control_size);
int check_control(char* control, char* buffer, int size);

int openFile(const char *name, int mode);
long readFileBytes(char* result, long size_to_read);
long readFileInfo();
int writeFileBytes(char* data, long size);
#endif