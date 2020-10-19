#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "application.h"


applicationLayer app;

int llopen(char* port, int mode) {
    int fd;
    set_port(port, &fd);

    if(mode == SENDER) {
        
    } else if(mode == RECEIVER) {
        read_set(fd);
    }

    return fd;
}

void set_port(char* port, int *fd) {
    struct termios oldtio, newtio;

    *fd = open(port, O_RDWR | O_NOCTTY);
	if (fd < 0){
		perror(port);
		exit(-1);
	}

	if (tcgetattr(*fd, &oldtio) == -1){ /* save current port settings */
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

	tcflush(*fd, TCIOFLUSH);

	if (tcsetattr(*fd, TCSANOW, &newtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");
}

int llread(int fd, unsigned char* buf) {

}

int main(int argc, char **argv) {

    int mode;

    if ((argc < 3) ||
		((strcmp("/dev/ttyS10", argv[2]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[2]) != 0)) || 
         (strcmp("read", argv[1]) != 0) &&
         (strcmp("write", argv[1])))
	{
		printf("Usage:\tnserial read/write SerialPort\n\tex: nserial read /dev/ttyS1\n");
		exit(1);
	}

    if(strcmp(argv[1], "read") == 0) {
        mode = RECEIVER;
        app.fileDescriptor = llopen(argv[2], mode);
    }
    else {
        mode = SENDER;
        app.fileDescriptor = llopen(argv[2], mode);
    }


}