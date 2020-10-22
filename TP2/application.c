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

#define MAX_CHUNK_SIZE 500

long readFileBytes(const char *name, char* result, long offset, long size_to_read){  //TODO: Check for errors
    FILE *fl = fopen(name, "rb");  
	if(fl == NULL){
		printf("Failed to open file\n");
		exit(-1);
	}

	printf("OFFSET: %d\n", offset);
	fseek(fl, offset, SEEK_SET);
    long len = fread(result, 1, size_to_read, fl); 
    fclose(fl);  
    return len;  
}

long readFileInfo(const char *name){
	FILE *fl = fopen(name, "rb");
	fseek(fl, 0, SEEK_END);
	long len = ftell(fl);
	fclose(fl);
	return len;
}

int writeFileBytes(const char *name, long size, char* data){  //TODO: Check for errors
	printf("NAME %s\n", name);
	FILE* fl = fopen(name, "wb");

	if(fl == NULL){
		printf("Couldnt open file for writing\n");
		exit(-1);
	}

    fwrite(data, 1, size, fl);
	fclose(fl);
    return 0;  
}

// int send_data(unsigned char* data, int length) {
// 	unsigned char packet[length + 4];

// 	int size = assemble_data_packet(data, length, packet);

// 	if(llwrite(app.fileDescriptor, packet, size) == -1) {
// 		printf("Error sending data packet");
// 		return -1;
// 	}

// 	return 0;
// }

int send_control(int type, char *filename, int fileSize) {
	unsigned char packet[strlen(filename) + 9]; //control+type,length,filename+type,lenght,fileSize

	int size = assemble_control_packet(type, filename, fileSize, packet);

	if(llwrite(app.fileDescriptor, packet, size) == -1) {
		printf("Error sending control packet\n");
		return -1;
	}
	return 0;
}

int assemble_data_packet(char* data, int length, int sequenceN, char* packet) {

	packet[0] = C_DATA;
	packet[1] = sequenceN % 256;
 	packet[2] = length / 256;
	packet[3] = length % 256;

	int i = 0;
	for(; i < length; ++i) {
		packet[i + 4] = data[i]; 
	}

	return i + 4;
}

int assemble_control_packet(int type, char *filename, int fileSize, char* packet) {

	if(type == START_C) 
		packet[0] =	C_START;
	else 
		packet[0] = C_END;
	
	packet[1] = FILE_SIZE;
	packet[2] = sizeof(fileSize);

	//TODO : improve 
	packet[3] = (fileSize >> 24) & 0xff;
	packet[4] = (fileSize >> 16) & 0xff;
	packet[5] = (fileSize >> 8) & 0xff;
	packet[6] = fileSize & 0xff;

	packet[7] = FILE_NAME;

	char ff[] = "pinguim2.gif"; //TODO : CAREFUL, HARDCODED
	// char ff[] = "windoh2.webm";

	packet[8] = strlen(ff);

	int i = 0;
	for(; i < strlen(ff); ++i) {
		packet[9 + i] = ff[i];
	}


	//Other way around

	// packet[1] = FILE_NAME;
	// packet[2] = strlen(ff);
	// int i = 0;
	// for(; i < strlen(ff); ++i) {
	// 	packet[3 + i] = ff[i];
	// }

	// packet[i + 3] = FILE_SIZE;
	// packet[i + 4] = sizeof(fileSize);
	// packet[i + 5] = (fileSize >> 24) & 0xff;
	// packet[i + 6] = (fileSize >> 16) & 0xff;
	// packet[i + 7] = (fileSize >> 8) & 0xff;
	// packet[i + 8] = fileSize & 0xff;

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

int read_control(char* control, char* fileName){
	int read_size;
	if((read_size = llread(app.fileDescriptor, control)) < 0){
			printf("--Error reading--\n");
			exit(-1);
	}

	if(control[0] != C_START && control[0] != C_END){
		printf("Invalid control\n");
		exit(-1);
	}

	int file_size = 0;
	char size[4];
	bzero(size, 4);
	if(control[1] == FILE_SIZE){
		int block_size = control[2];
		int j = block_size - 1;
		for(int i = 0; i < block_size; i++){
			int part = ((unsigned char)(control[i+3]) << (j * 8));
			file_size |= part;
			j--;
		}

		int name_size = control[block_size + 4];
		for(int i = 0; i < name_size; i++){
			fileName[i] = control[i + block_size + 5];
		}
	} else if(control[1] == FILE_NAME){
		int name_size = control[2];
		for(int i = 0; i < name_size; i++){
			fileName[i] = control[i + 3];
		}

		int block_size = control[name_size + 4];
		int j = block_size - 1;
		for(int i = 0; i < block_size; i++){
			file_size |= ((unsigned char)(control[name_size + i + 5]) << (j * 8));
			j--;
		}
	}

	return file_size;
}

int main(int argc, char **argv) {

    if ((argc < 4) ||
		((strcmp("/dev/ttyS10", argv[3]) != 0) &&
		 (strcmp("/dev/ttyS11", argv[3]) != 0)) || 
         ((strcmp("read", argv[1]) != 0) &&
         (strcmp("write", argv[1]))))
	{
		printf("Usage:\tnserial read/write filename SerialPort\n\tex: nserial read pinguim.gif /dev/ttyS1\n");
		exit(1);
	}

    if(strcmp(argv[1], "read") == 0)
        app.status = RECEIVER;
	else 
        app.status = SENDER;

	if((app.fileDescriptor = llopen(argv[3], app.status)) < 0){
		printf("GG\n");
		exit(-1);
	}

	char file_name[1024];
	strcpy(file_name, argv[2]);
	
	if(app.status == RECEIVER){

		char control[1024];
		char buffer[1024];
		int read_size;

		int file_size = read_control(control, file_name);

		char* file_buffer = (char*)malloc(sizeof(char)*file_size);
		
		int curr_index = 0;

		int control_found = 0;


		while(control_found == 0){
			if((read_size = llread(app.fileDescriptor, buffer)) < 0){
				printf("--Error reading--\n");
				free(file_buffer);
				exit(-1);
			}

			//TODO CHANGE LATER
			if(buffer[0] == C_END){
				control_found = 0;
				break;
			} 

			int packet_size = (unsigned char)(buffer[2]) * 256 + (unsigned char)(buffer[3]);
			printf("PACKET SIZE: %ld\n", packet_size);


			for(int i = 4; i < packet_size + 4; i++){
				file_buffer[curr_index + i - 4] = buffer[i];
			}

			//printf("FILE DATA: %s\n", file_buffer);

			curr_index += packet_size;
		}
		writeFileBytes(file_name, file_size, file_buffer);


		free(file_buffer);


		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}

	} else {

		long file_size = readFileInfo(file_name);
		long curr_index = 0;
		char file[MAX_CHUNK_SIZE];
		//send_control(START, filename, fileSize);
		send_control(START_C, file_name, file_size);

		int size_remaining = file_size;
		int size_to_send;

		while(curr_index < file_size){
			
			if(size_remaining < MAX_CHUNK_SIZE){
				size_to_send = readFileBytes(file_name, file, curr_index, size_remaining);
			} else {
				size_to_send = readFileBytes(file_name, file, curr_index, MAX_CHUNK_SIZE);
			}

			//printf("FILE DATA: %s\n", file);
			
			curr_index += size_to_send;
			
			size_remaining -= size_to_send;

			char packet[size_to_send + 4];
			int packet_size = assemble_data_packet(file, size_to_send, size_to_send, packet); //TODO: sequenceN
			printf("PACKET SIZE %d\n", packet_size);

			if(llwrite(app.fileDescriptor, packet, packet_size) < 0){
				printf("--Error writing--\n");
				exit(-1);
			}
		}

		//ler dados do ficheiro e chamar send_data()
		send_control(END_C, file_name, file_size);
		printf("Sent Control\n");

		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}
	}

	return 0;
}