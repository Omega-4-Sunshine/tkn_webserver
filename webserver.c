#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

#define MYPORT "3490"
#define BACKLOG 10
#define BUFFER_CAPACITY 16000

// https://en.wikipedia.org/wiki/C_data_types
// COMPILE + RUN: make -C build && ./build/webserver 

//printf 'GET /index.html HTTP/1.1\r\nHost: localhost:1234\r\nConnection: close\r\n\r\n GET /index.html HTTP/1.1\r\nHost: localhost:1234\r\nConnection: close\r\n\r\n' | nc 127.0.0.1 1234

//git push -u origin main


enum RequestType {
    GET,
    POST,
    PUT,
    DELETE,
    INCORRECT
} typedef RequestType;

struct RequestInfo {
    RequestType requestType;
    char path[1024];
    char version[16];
    int content_length;
} typedef RequestInfo;


// function declarations:
void display_info(RequestInfo info);
void handle_http_request(char buffer[], int new_fd);
void handle_socket_transaction(int new_fd, char output[], int output_valid_bytes);
void clear_one_request_buffer(char buffer[], char* request_end_ptr, int* output_valid_bytes);
RequestInfo find_Request_Info(char buffer[]);



int main(int argc, char* port_in[]) { //arr at [0] is name / Aufrufpfad Programm, [1] is first argument

    if(argc != 3) {
        printf("number of input arguments incorrect");
        return -1;
    }


    struct sockaddr_storage storage;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *res;

    int sockfd, new_fd; //file descriptors

    // -- set up hint for getaddrinf()
    memset(&hints, 0, sizeof(hints)); //set all to zeros
    hints.ai_family = AF_INET; //automatic IPv4 /IPv6 //gerdae nur IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(port_in[1], port_in[2], &hints, &res) != 0) {
        perror("addrInfo");
        exit(1);
    } 

    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    
    if(bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        exit(1);
    }

    if(listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    fprintf(stderr, "Server listening on %s:%s\n",
        port_in[1], port_in[2]);
    fflush(stderr);



    addr_size = sizeof storage;

    int output_valid_bytes = 0;
    char output[BUFFER_CAPACITY];
    memset(output, 0, sizeof(output)); // setzt \0 überall


    while(1) {
        if((new_fd = accept(sockfd, (struct sockaddr *)&storage, &addr_size)) == -1) {
            perror("accept");
            exit(1);
        }

        handle_socket_transaction(new_fd, output, output_valid_bytes);
        
        close(new_fd);
    }

    

}

RequestInfo find_Request_Info(char buffer[]) {
    RequestInfo info;

    info.content_length = -1;

    char method[16];
    char path[1024];
    char version[16];


    if(sscanf(buffer, "%15s %1023s %15s", method, path, version) != 3) { //returns number of arguments read
        printf("2\n");
        info.requestType = INCORRECT;
    }

    int start_length = strlen(method) + strlen(path) + strlen(version); //does not include \r\n jeweils ()
    start_length = start_length + strlen("\r\n\r\n");

    if(strlen(method) == 0 || strlen(path) == 0 || strlen(version) == 0) {
        info.requestType = INCORRECT;
    }

    if(strcmp(method, "GET") == 0) {
        info.requestType = GET;
    } else if(strcmp(method, "PUT") == 0) {
        info.requestType = PUT;
    } else if(strcmp(method, "DELETE") == 0) {
        info.requestType = DELETE;
    } else if(strcmp(method, "POST") == 0) {
        info.requestType = POST;
    } else {
        info.requestType = INCORRECT;
        printf("3\n");
        return info;
    }

    strcpy(info.path,path);
    strcpy(info.version,version);

    strcpy(info.path, buffer + start_length);

    //look at headers <key>: <value>

    char* curent_pos = buffer + start_length;

    while(1) {
        char key[1024];
        memset(key,0,1023);
        char value[1024];
        memset(value,0,1023);

        int read = sscanf(curent_pos, "%1023s %1023s",key, value);
        if(read == 1 || read > 2) {
            info.requestType = INCORRECT;
            printf("%s",key);
            printf("4\n");
            return info;
        } else if (read == 0) {
            break;
        } else if (read == EOF) {
            break;
        }

        curent_pos = curent_pos + (strlen(key) + strlen(value) + 3); //Leerzeichen + \r\n


        if(key[strlen(key) - 1] != ':') {
            printf("TESTEST: %s \n",key);
            info.requestType = INCORRECT;
            return info;
        }

        if(strcmp(key,"Content-Length:") == 0) {
            int i;
            sscanf(value, "%i", &i);
            info.content_length = i;
        }

    }
    
    return info;

}

void display_info(RequestInfo info) {
    printf("-------\n");
    printf("Content Length: %i\n",info.content_length);
    printf("Request Type: %i\n", info.requestType);
    printf("-------\n");
}

void handle_http_request(char buffer[], int new_fd) {

    RequestInfo info = find_Request_Info(buffer);

    display_info(info);

    //check if INCORRECT, bc. all other values then undefined 

    
    char response[1024];
    memset(response, 0, 1024);

    switch(info.requestType) {
        case INCORRECT:
            strcpy(response, "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n");  
        break;
        case GET:
            printf("GET SUCCESFULL\n");
            strcpy(response, "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n");        
        break;
        default:
            strcpy(response, "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n");   
        break;
    }

    if(send(new_fd, response, sizeof(response) - 1, 0) == -1) {
        perror("send");
        exit(1);
    }

}



void handle_socket_transaction(int new_fd, char output[], int output_valid_bytes) {
    while(1) {
        int bytes_send = recv(new_fd, output + output_valid_bytes * sizeof(char), BUFFER_CAPACITY - output_valid_bytes - 1, 0); 
        if(bytes_send < 0) {
            perror("recv");
            exit(1);
            return;
        } else if (bytes_send == 0) {
            memset(output,0, BUFFER_CAPACITY);
            printf("end of transmission reached\n");
            return;
        }

        output_valid_bytes = output_valid_bytes + bytes_send;
        output[output_valid_bytes] = '\0';


        while(1) { //check for complete http requests
            char* request_end = strstr(output, "\r\n\r\n");

            char sanitized[BUFFER_CAPACITY];
            strcpy(sanitized,output);
            *(strstr(sanitized, "\r\n\r\n") + 4) = '\0';
            

            if(request_end != NULL) { // one message send completly
                handle_http_request(sanitized, new_fd);

                clear_one_request_buffer(output, request_end, &output_valid_bytes);
            } else {
                break;
            }
        } 
    }
}


void clear_one_request_buffer(char buffer[], char* request_end_ptr, int* output_valid_bytes) {
    char* request_end = request_end_ptr + 4 * sizeof(char);

    int i = 0;
    while(*(request_end + i) != '\0' && i < BUFFER_CAPACITY) {
        buffer[i] = *(request_end + i);
        i++;
    }
    *output_valid_bytes = i;
    while(i < BUFFER_CAPACITY) {
        buffer[i] = '\0';
        i++;
    }
}



