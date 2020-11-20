#include <stdlib.h>
#include <stdio.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define CONNECTION_PORT 21
#define ANONYMOUS_PASSWORD "1234"

typedef struct {
    char* host_name;
    char* url;
    char* host_ip;
    char* username;
    char* password;
    int anonymous;
    int sockfd;
    int data_port;
    int datafd;
} connection;

void init_connection(connection *conn) {
    conn->host_name = NULL;
    conn->url = NULL;
    conn->host_ip = NULL;
    conn->username = NULL;
    conn->password = NULL;
    conn->anonymous = -1;
    conn->sockfd = -1;
    conn->data_port = -1;
    conn->datafd = -1;
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
    if(conn->sockfd != -1)
        close(conn->sockfd);
    if(conn->datafd != -1)
        close(conn->datafd);
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
        fprintf(stderr, "Not ftp protocol\n");
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

int parse_pasv_response(connection *conn, char* message){
    char* messageptr;
    strtok_r(message, ",", &messageptr);
    strtok_r(NULL, ",", &messageptr);
    strtok_r(NULL, ",", &messageptr);
    strtok_r(NULL, ",", &messageptr);

    char* l1_str = strtok_r(NULL, ",", &messageptr);
    char* l2_str = strtok_r(NULL, ",", &messageptr);

    int L1 = atoi(l1_str);
    int L2 = atoi(l2_str);

    conn->data_port = L1 * 256 + L2;

    // printf("%d\n", conn->data_port);
    return 0;
}

int make_connection(connection *conn){

	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(conn->host_ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(CONNECTION_PORT);		    /*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((conn->sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        fprintf(stderr, "Failed to open socket\n");
        return -1;
    }
	/*connect to the server*/
    if(connect(conn->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "Failed to connect to server\n");
        return -1;
    }

    return 0;
}

int make_data_connection(connection *conn){

	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(conn->host_ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(conn->data_port);		    /*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((conn->datafd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        fprintf(stderr, "Failed to open socket\n");
        return -1;
    }
	/*connect to the server*/
    if(connect(conn->datafd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "Failed to connect to server\n");
        return -1;
    }

    return 0;
}

int send_message(connection *conn, char* message){
    if(send(conn->sockfd, message, strlen(message), 0) < 0){
        return -1;
    }
	printf("Sent %ld bytes: %s\n", strlen(message), message);
    return 0;
}

int read_message(connection *conn, char* message){

    //TODO REFACTOR GETS STUCK ON FIRST MESSAGE SOMETIMES
    int read_bytes = -1;
    do {
        read_bytes = recv(conn->sockfd, message, 4096, 0);
        message[read_bytes] = '\0';
        if(read_bytes < 0){
            return -1;
        }
        // printf("Received %ld bytes: %s\n", strlen(message), message);
        // printf("Char: %c\n", message[3]);
        printf("%s", message);
    } while (read_bytes > 3 && message[3] == '-');

    return read_bytes;
}

int read_data(connection *conn){

    //Used to get the name of the file without its path
    char* urlptr;
    char* token = strtok_r(conn->url, "/", &urlptr);
    char* result;

    do {
        result = token;
        token = strtok_r(NULL, "/", &urlptr);
    } while(token != NULL);


	FILE* fd;
    if((fd = fopen(result, "w")) == NULL){
        fprintf(stderr, "Error opening file\n");
        return -1;
    }

    int i = 0;
    char c;
    int read_bytes = -1;
    char* message = (char*)malloc(sizeof(char) * 200);
    int cur_size = 200;
    while(1) {
        read_bytes = recv(conn->datafd, &c, 1, 0);
        if(read_bytes < 0)
            return -1;
        if(read_bytes == 0)
            break;
        if(i == cur_size){
            message = (char*)realloc(message, cur_size * 2);
            cur_size *= 2;
        }
        message[i] = c;
        i += read_bytes;
    }

    if(fwrite(message, 1, i, fd) < 0){
		perror("Error writing to file\n");
		return -1;
	}

    free(message);
    fclose(fd);
    printf("Finished reading file\n");
    return 0;  

}

int main(int argc, char **argv){

    if(argc != 2) {
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

    if(make_connection(&conn) < 0){
        fprintf(stderr, "Error making connection\n");
        cleanup(&conn);
        exit(-1);
    }
    
    //Does not work everytime
    //Greeting
    char message[4096];
    if(read_message(&conn, message) < 0 && message[0] != '2'){
        fprintf(stderr, "Error reading greeting\n");
        cleanup(&conn);
        exit(-1);
    }

    char user_msg[256];
    strcpy(user_msg, "user ");
    if(conn.anonymous == 1)
        strcat(user_msg, "anonymous\n");
    else {
        strcat(user_msg, conn.username);
        strcat(user_msg, "\n");
    }
    
    //Send username
    if(send_message(&conn, user_msg) < 0){
        fprintf(stderr, "Failed sending username\n");
        cleanup(&conn);
        exit(-1);
    }

    //Read password prompt
    if(read_message(&conn, message) < 0 && message[0] != '3'){
        fprintf(stderr, "Error reading\n");
        cleanup(&conn);
        exit(-1);
    }

    if(message[0] == '5'){
        fprintf(stderr, "Error: server is anonymous only\n");
        cleanup(&conn);
        exit(-1);
    }


    char password_msg[256];
    strcpy(password_msg, "pass ");
    if(conn.anonymous == 1){
        strcat(password_msg, ANONYMOUS_PASSWORD);
        strcat(password_msg, "\n");
    } else {
        strcat(password_msg, conn.password);
        strcat(password_msg, "\n");
    }
    
    //Send username
    if(send_message(&conn, password_msg) < 0){
        fprintf(stderr, "Failed sending password\n");
        cleanup(&conn);
        exit(-1);
    }

    //Password reponse
    if(read_message(&conn, message) < 0 && message[0] != '2'){
        fprintf(stderr, "Error reading\n");
        cleanup(&conn);
        exit(-1);
    }

    //Enter passive
    if(send_message(&conn, "pasv\n") < 0){
        fprintf(stderr, "Failed sending pasv command\n");
        cleanup(&conn);
        exit(-1);
    }

    //Read pasv response
    if(read_message(&conn, message) < 0 && message[0] != '2'){
        fprintf(stderr, "Error reading\n");
        cleanup(&conn);
        exit(-1);
    }

    parse_pasv_response(&conn, message);

    if(make_data_connection(&conn) < 0){
        fprintf(stderr, "Error making connection\n");
        cleanup(&conn);
        exit(-1);
    }


    //Enter passive
    char retrieve_msg[256];
    strcpy(retrieve_msg, "retr ");
    strcat(retrieve_msg, conn.url);
    strcat(retrieve_msg, "\n");
    if(send_message(&conn, retrieve_msg) < 0){
        fprintf(stderr, "Failed sending pasv command\n");
        cleanup(&conn);
        exit(-1);
    }

    if(read_data(&conn) < 0){
        fprintf(stderr, "Error reading\n");
        cleanup(&conn);
        exit(-1);
    }


    cleanup(&conn);

    return 0;
}

