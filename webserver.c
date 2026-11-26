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

// function declarations:
void handle_http_request(char buffer[]);
void handle_socket_transaction(int new_fd, char output[], int output_valid_bytes);
void clear_one_request_buffer(char buffer[], char* request_end_ptr, int* output_valid_bytes);

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

void handle_http_request(char buffer[]) {
    char method[16];
    char path[1024];
    char version[16];

    sscanf(buffer, "%15s %1023s %15s", method, path, version);

    printf("------\n");
    printf("%s\n", method);
    printf("%s\n", path);
    printf("%s\n", version); 
    printf("------\n");
}

void handle_socket_transaction(int new_fd, char output[], int output_valid_bytes) {
    printf("TESTEST\n");
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

        printf("%i\n",bytes_send);

        output_valid_bytes = output_valid_bytes + bytes_send;
        output[output_valid_bytes] = '\0';


        while(1) { //check for complete http requests
            char* request_end = strstr(output, "\r\n\r\n");
            if(request_end != NULL) { // one message send completly
                handle_http_request(output);
                const char response[] =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n";
                if(send(new_fd, response, sizeof(response) - 1, 0) == -1) {
                    perror("send");
                    exit(1);
                }
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



