#include <stdlib.h>
#include <stdio.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>

int get_ip(char* host, char** address){

    struct hostent *h;

    if ((h = gethostbyname(host)) == NULL) {
        return -1;
    }

    *address = malloc(sizeof(char) * h->h_length);

    //Maybe improve later
    strcpy(*address, inet_ntoa(*((struct in_addr *)h->h_addr)));
    return 0;
}

int main(int argc, char **argv){

    if (argc != 2) {
        perror("usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    char* address;
    if(get_ip(argv[1], &address) < 0){
        perror("Failed to get host ip\n");
        exit(-1);
    }

    printf("Address: %s\n", address);

    free(address);

    return 0;
}

