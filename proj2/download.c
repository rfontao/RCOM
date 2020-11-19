#include <stdlib.h>
#include <stdio.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char* host_name;
    char* url;
    char* host_ip;
    char* username;
    char* password;
    int anonymous;
} connection;

void init_connection(connection *conn) {
    conn->host_name = NULL;
    conn->url = NULL;
    conn->host_ip = NULL;
    conn->username = NULL;
    conn->password = NULL;
    conn->anonymous = -1;
}

void cleanup(connection *conn){
    if(conn->host_name != NULL)
        free(conn->host_name);
    if(conn->url != NULL)
        free(conn->url);
    if(conn->host_ip != NULL)
        free(conn->host_ip);
    if(conn->username != NULL)
        free(conn->username);
    if(conn->password != NULL)
        free(conn->password);
}

void print_connection(connection* conn){
    printf("------------------\n");
    printf("Connection Info:\n");
    printf("Host name: %s\n", conn->host_name);
    printf("Host IP: %s\n", conn->host_ip);
    printf("URL: %s\n", conn->url);

    if(conn->anonymous){
        printf("Anonymous: TRUE\n");
    } else {
        printf("Username: %s\n", conn->username);
        printf("Password: %s\n", conn->password);

    }

    printf("------------------\n");
}


int get_ip(connection *conn){
 
    struct hostent *h;

    if ((h = gethostbyname(conn->host_name)) == NULL) {
        return -1;
    }

    conn->host_ip = (char*)malloc(sizeof(char) * h->h_length);

    //Maybe improve later
    strcpy(conn->host_ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    return 0;
}

int parse_input(connection *conn, char *input) {
    const char s_login[2] = "@";
    const char s[2] = "/";
    const char s_url[2] = "";

    char* token;
    char* inputptr;
    token = strtok_r(input, s, &inputptr);
    if(token == NULL) 
            return -1;
    if(strcmp(token, "ftp:") != 0){
        perror("Not ftp protocol\n");
        return -1;
    }

    token = strtok_r(NULL, s, &inputptr);
    if(token == NULL) 
        return -1;

    char* auxptr;
    char* aux = (char*)malloc((sizeof(char) * strlen(token)) + 1);
    strcpy(aux, token);
    char* aux1 = strtok_r(aux, s_login, &auxptr);
    if(strcmp(token, aux1) == 0){
        conn->anonymous = 1;
    } else 
        conn->anonymous = 0;

    free(aux);

    if(conn->anonymous == 0) {
        char* aux1 = (char*)malloc((sizeof(char) * strlen(token)) + 1);
        strcpy(aux1, token);

        char* infoptr;
        char* info = strtok_r(token, s_login, &infoptr);

        if(info == NULL) {
            free(aux1);
            return -1;
        }

        // printf("%s\n", info);
        // printf("%s\n", token);
        if(strcmp(info, aux1) == 0){
            free(aux1);
            return -1;
        }
        free(aux1);

        char* aux2 = (char*)malloc((sizeof(char) * strlen(info)) + 1);
        strcpy(aux2, info);

        char* loginptr;
        char* loginInfo = strtok_r(info, ":", &loginptr);
        if(loginInfo == NULL) {
            free(aux2);
            return -1;
        }

        if(strcmp(loginInfo, aux2) == 0){
            free(aux2);
            return -1;
        }
        free(aux2);

        conn->username = (char*)malloc((sizeof(char) * strlen(loginInfo)) + 1);
        strcpy(conn->username, loginInfo);
        
        loginInfo = strtok_r(NULL, "", &loginptr);
        if(loginInfo == NULL)
            return -1;
        conn->password = (char*)malloc((sizeof(char) * strlen(loginInfo)) + 1);
        strcpy(conn->password, loginInfo);
        
        token = strtok_r(NULL, s, &infoptr);
        if(token == NULL) 
            return -1;
    }

    //Not anonymous and end of anonymous
    conn->host_name = (char*)malloc((sizeof(char) * strlen(token)) + 1);
    strcpy(conn->host_name, token);

    token = strtok_r(NULL, s_url, &inputptr);
    if(token == NULL) 
        return -1;
    conn->url = (char*)malloc((sizeof(char) * strlen(token)) + 1);
    strcpy(conn->url, token);
    
    return 0;
}

int main(int argc, char **argv){

    if (argc != 2) {
        fprintf(stderr, "usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    connection conn;
    init_connection(&conn);
    
    if(parse_input(&conn, argv[1]) < 0){
        fprintf(stderr, "Error parsing input\n");
        fprintf(stderr, "usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        cleanup(&conn);
        exit(-1);
    }

    print_connection(&conn);

    if(get_ip(&conn) < 0){
        fprintf(stderr, "Failed to get host ip\n");
        cleanup(&conn);
        exit(-1);
    }

    print_connection(&conn);

    cleanup(&conn);

    return 0;
}

