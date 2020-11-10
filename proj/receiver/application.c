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
#include "common.h"

applicationLayer app;
struct termios oldtio, newtio;

#define MAX_CHUNK_SIZE 5000

int openFile(const char *name, int mode){
	if(mode == RECEIVER){
		app.file = fopen(name, "wb");
	} else if(mode == SENDER){
		app.file = fopen(name, "rb");
	} else {
		printf("Invalid mode\n");
		return -1;
	} 
	if(app.file == NULL){
		printf("Failed to open file\n");
		return -1;
	}
	return 0;
}

long readFileBytes(char* result, long size_to_read){
	long len;
	if((len = fread(result, 1, size_to_read, app.file)) == 0){
		perror("Failed to read from file\n");
		return -1;
	} 
    return len;
}

long readFileInfo(){
	struct stat buf;
	int fd = fileno(app.file);
	fstat(fd, &buf);
	// fseek(app.file, 0, SEEK_END);
	// long len = ftell(app.file);
	// fseek(app.file, 0, SEEK_SET);
	return buf.st_size;
}

int writeFileBytes(char* data, long size){
	if(fwrite(data, 1, size, app.file) < 0){
		perror("Error writing to file\n");
		return -1;
	}
    return 0;  
}

int send_control(int type, char *filename, long fileSize) {
	char packet[strlen(filename) + 9];

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

int assemble_control_packet(int type, char* filename, long fileSize, char* packet) {

	if(type == START_C) 
		packet[0] =	C_START;
	else 
		packet[0] = C_END;
	
	packet[1] = FILE_SIZE;
	packet[2] = sizeof(fileSize);

	int k = 3;
	for(; k < sizeof(fileSize); k++) {
		packet[k] = (fileSize >> ((k-3)*8)) & 0xff;
	}

	packet[k++] = FILE_NAME;
	packet[k++] = strlen(filename);

	int i = 0;
	for(; i < strlen(filename); ++i) {
		packet[k + i] = filename[i];
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

	return k + i;
}

int llopen(char* port, int mode) {
	printf("%s", port);
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

int llread(int fd, char** buffer){
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

int read_control(char** ctl, char* fileName, int* control_size){
	if((*control_size = llread(app.fileDescriptor, ctl)) < 0){
		printf("--Error reading--\n");
		free(*ctl);
		exit(-1);
	}

	char* control = *ctl;

	printf("CONTROLLL\n");
	print_frame(control, *control_size);

	if(control[0] != C_START && control[0] != C_END){
		printf("Invalid control\n");
		exit(-1);
	}

	long file_size = 0;
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

int check_control(char* control, char* buffer, int size){
	// print_frame(control, size);
	// print_frame(buffer, size);

	for(int i = 1; i < size; i++){
		if(control[i] != buffer[i]){
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	int c;
	int file_is_set = 0, mode_is_set = 0, port_is_set = 0;
	char file_name[1024] = "";
	char port_name[1024] = "";

	while ((c = getopt(argc, argv, "F:P:M:")) != -1) {
		switch (c) {
		case 'F':
			strcpy(file_name, optarg);
			file_is_set = 1;
			break;
		case 'P':
			strcpy(port_name, optarg);
			port_is_set = 1;
			break;
		case 'M':
			if(strcmp(optarg, "read") == 0 || strcmp(optarg, "Read") == 0 || strcmp(optarg, "R") == 0 || strcmp(optarg, "r") == 0){
				app.status = RECEIVER;
			} else if(strcmp(optarg, "write") == 0 || strcmp(optarg, "Write") == 0 || strcmp(optarg, "W") == 0 || strcmp(optarg, "w") == 0){
				app.status = SENDER;
			} else {
				printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
				exit(-1);
			}
			mode_is_set = 1;
			break;
		case '?': // invalid option
			printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
			exit(-1);
			break;
		default:
			printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
			exit(-1);
			break;
		}
	}

	if(port_is_set == 0){
		printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
		exit(-1);
	}

	if(mode_is_set == 0){
		printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
		exit(-1);
	}

	if((file_is_set == 0) && app.status == SENDER){
		printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
		exit(-1);
	}

	if((file_is_set == 1) && app.status == RECEIVER){
		printf("Usage : %s -M read/write -P port -F file (only when write mode is specified)\n", argv[0]);
		exit(-1);
	}

	if((app.fileDescriptor = llopen(port_name, app.status)) < 0){
		printf("Failed to open file\n");
		exit(-1);
	}
	
	if(app.status == RECEIVER){

		char* control;
		char* buffer;
		int read_size;
		int control_size;

		// int file_size = read_control(&control, file_name);
		read_control(&control, file_name, &control_size);

		char file_buffer[MAX_CHUNK_SIZE];
		
		if(openFile(file_name, app.status) < 0){
			perror("Error opening file\n");
			free(control);
			exit(-1);
		}

		int control_found = 0;
		int sequenceN = 0;

		while(control_found == 0){
			if((read_size = llread(app.fileDescriptor, &buffer)) < 0){
				printf("--Error reading--\n");
				free(control);
				free(buffer);
				fclose(app.file);
				exit(-1);
			}

			if(buffer[0] == C_END){
				//End buffer is different from start buffer
				if(check_control(control, buffer, control_size) < 0){
					printf("--End buffer is different from start buffer--\n");
					free(control);
					free(buffer);
					fclose(app.file);
					exit(-1);
				}
				control_found = 0;
				free(buffer);
				break;
			}
			
			//Check for sequence number
			if((unsigned char)(buffer[1]) != sequenceN){
				printf("--Received packet with wrong sequence number. Aborting--\n");
				free(control);
				free(buffer);
				fclose(app.file);
				exit(-1);
			}

			int packet_size = (unsigned char)(buffer[2]) * 256 + (unsigned char)(buffer[3]);
			// printf("PACKET SIZE: %d\n", packet_size);

			for(int i = 4; i < packet_size + 4; i++){
				file_buffer[i - 4] = buffer[i];
			}
			free(buffer);
			//printf("FILE DATA: %s\n", file_buffer);
			writeFileBytes(file_buffer, packet_size);

			sequenceN = (sequenceN + 1) % 255;
		}

		free(control);
		fclose(app.file);

		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}

	} else if(app.status == SENDER) {

		if(openFile(file_name, app.status) < 0){
			perror("Error opening file\n");
			exit(-1);
		}

		long file_size = readFileInfo();
		char file[MAX_CHUNK_SIZE];

		send_control(START_C, file_name, file_size);

		int size_remaining = file_size;
		int size_to_send;
		int sequenceN = 0;

		while(size_remaining > 0){
			
			if(size_remaining < MAX_CHUNK_SIZE){
				if((size_to_send = readFileBytes(file, size_remaining)) < 0){
					perror("Error reading file\n");
					fclose(app.file);
					exit(-1);
				}
			} else {
				if((size_to_send = readFileBytes(file, MAX_CHUNK_SIZE)) < 0){
					perror("Error reading file\n");
					fclose(app.file);
					exit(-1);
				}
			}
			
			size_remaining -= size_to_send;

			char* packet = (char*)malloc(size_to_send + 4);
			int packet_size = assemble_data_packet(file, size_to_send, sequenceN, packet);
			// printf("PACKET SIZE %d\n", packet_size);

			if(llwrite(app.fileDescriptor, packet, packet_size) < 0){
				printf("--Error writing--\n");
				free(packet);
				fclose(app.file);
				exit(-1);
			}

			sequenceN = (sequenceN + 1) % 255;
			free(packet);
		}

		//ler dados do ficheiro e chamar send_data()
		send_control(END_C, file_name, file_size);
		printf("Sent Control\n");

		fclose(app.file);

		if(llclose(app.fileDescriptor) < 0){
			printf("Failed closing\n");
			exit(-1);
		}
	}

	return 0;
}