#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "application.h"
#include "message_macros.h"
#include "read.h"
#include "write.h"

applicationLayer app;
struct termios oldtio, newtio;

int send_data(unsigned char* data, int length) {
	unsigned char packet[length + 4];

	int size = assemble_data_packet(data, length, packet);

	if(llwrite(app.fileDescriptor, packet, size) == -1) {
		printf("Error sending data packet");
		return -1;
	}

	return 0;
}

int send_control(int type, char *filename, int fileSize) {
	unsigned char packet[strlen(filename) + 9]; //control+type,length,filename+type,lenght,fileSize

	int size = assemble_control_packet(type, filename, fileSize, packet);

	if(llwrite(app.fileDescriptor, packet, size) == -1) {
		printf("Error sending control packet\n");
		return -1;
	}
	return 0;
}

int assemble_data_packet(unsigned char* data, int length, unsigned char* packet) {

	static int sequenceN = 0;

	packet[0] = C_DATA;
	packet[1] = sequenceN % 256;
 	packet[2] = length / 256;
	packet[3] = length % 256;

	int i = 0;
	for(; i < length; ++i) {
		packet[i + 4] = data[i]; 
	}

	sequenceN++;

	return i + 4;
}

int assemble_control_packet(int type, char *filename, int fileSize, unsigned char* packet) {

	if(type = START) 
		packet[0] =	C_START;
	else 
		packet[0] = C_END;

	packet[1] = FILE_SIZE;
	packet[2] = sizeof(fileSize);

	packet[3] = (fileSize >> 24) & 0xff;
	packet[4] = (fileSize >> 16) & 0xff;
	packet[5] = (fileSize >> 8) & 0xff;
	packet[6] = fileSize & 0xff;

	packet[7] = FILE_NAME;
	packet[8] = strlen(filename);
	
	int i = 0;
	for(; i < strlen(filename); ++i) {
		packet[9 + i] = filename[i];
	}

	return 9 + i;
}

int llopen(char* port, int mode) {
    int fd = set_port(port);

    if(mode == SENDER) {
        if(send_set(fd) < 0)
			return -1;
    } else if(mode == RECEIVER) {
        read_set(fd);
    }

	printf("---Connection established---\n");

    return fd;
}

int llread(int fd, char* buffer){
	return read_info(fd, buffer);
}

int llwrite(int fd, char* buffer, int length){
	return send_info(fd, buffer, length);
}

int llclose(int fd){

	if(app.status == SENDER) {
        if(send_disc_sender(fd) < 0)
			return -1;
    } else if(app.status == RECEIVER) {
        read_disc(fd);
		if(send_disc_receiver(fd) < 0)
			return -1;
    }

	printf("---Connection ended---\n");

	if(close_port() < 0){
		printf("Error closing port\n");
		exit(-1);
	}

    return 0;
}

int set_port(char* port){

    int fd = open(port, O_RDWR | O_NOCTTY);
	if (fd < 0){
		perror(port);
		exit(-1);
	}

	if (tcgetattr(fd, &oldtio) == -1){ /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

    bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 1;	/* blocking read until 5 chars received */

	/* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	return fd;
}

int close_port(){
	sleep(1);
	if(tcsetattr(app.fileDescriptor, TCSANOW, &oldtio) < 0){
		return -1;
	}

	if(close(app.fileDescriptor) < 0){
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {

    int mode;

    if ((argc < 3) ||
		((strcmp("/dev/ttyS10", argv[2]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[2]) != 0)) || 
         ((strcmp("read", argv[1]) != 0) &&
         (strcmp("write", argv[1]))))
	{
		printf("Usage:\tnserial read/write SerialPort\n\tex: nserial read /dev/ttyS1\n");
		exit(1);
	}

    if(strcmp(argv[1], "read") == 0)
        app.status = RECEIVER;
	else 
        app.status = SENDER;

	if((app.fileDescriptor = llopen(argv[2], app.status)) < 0){
		printf("GG\n");
		exit(-1);
	}

	if(app.status == RECEIVER){

		char buffer[1024];
		int read_size;
		if((read_size = llread(app.fileDescriptor, buffer)) < 0){
			printf("--Error reading--\n");
			exit(-1);
		}

		write(STDOUT_FILENO, buffer, read_size);
		printf("\n");

		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}

	} else {
		char word[] = "Hello there";

		if(llwrite(app.fileDescriptor, word, 11) < 0){
			printf("--Error writing--\n");
			exit(-1);
		}

		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}
	}

	return 0;
}