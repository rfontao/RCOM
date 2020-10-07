/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 			"01111110"
#define SENDER_A 		"00000011"
#define SENDER_C_SET 	"00000011"
#define BCC1 			"00000000"

#define MAX_SET_RETRIES 3
#define SET_TIMEOUT		3

volatile int STOP = FALSE;

int fd; 	/*TODO: Change later*/

void write_to_port(int fd, void* data){
	int sent = write(fd, data, strlen(data) + 1);
	printf("%d bytes written\n", sent);
}

void send_set_frame(int fd){
	char set[41];
	/* SET asswembly */
	/* [F,A,C,BCC1,F] */
	strcpy(set, FLAG);
	strcat(set, SENDER_A);
	strcat(set, SENDER_C_SET);
	strcat(set, BCC1);
	strcat(set, FLAG);

	write_to_port(fd, set);
}

void sigalrm_handler(int sig){
	if(sig == SIGALRM){
		send_set_frame(fd);
	}
}


int main(int argc, char **argv){
	int res;
	struct termios oldtio, newtio;
	

	if ((argc < 2) ||
		((strcmp("/dev/ttyS10", argv[1]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[1]) != 0)))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
		exit(1);
	}

	/*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0){
		perror(argv[1]);
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

	//Setting the alarm handler
	struct sigaction action;
	action.sa_handler = sigalrm_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT,&action,NULL);

	printf("New termios structure set\n");

	char buf[255]; 

	// fgets(buf,255,stdin);
	// //Put \0 on the end of the read string
	// buf[strlen(buf) - 1] = '\0';

	//Sending the data
	int current_try = 0;

	send_set_frame(fd);

	while(current_try < MAX_SET_RETRIES){
		alarm(SET_TIMEOUT);

		char* result = (char*)malloc(sizeof(char));
		int a = 0;

		while (STOP == FALSE){
			res = read(fd, result, 1);
			if(a == 0)
				strcpy(buf, result);
			else
				strcat(buf, result);
			a++;
			if (result[0] == '\0')
				STOP = TRUE;
		}

		if(buf[0] == '0'){
			printf(":%s:%d\n", buf, a);
			break;
		}
		
		current_try++;
	}
	

	sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}


