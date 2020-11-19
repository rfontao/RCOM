#include <stdlib.h>
#include <stdio.h>


int main(int argc, char **argv){

    if (argc != 2) {  
        perror("usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    return 0;

}