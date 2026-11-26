#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define MYPORT "3490"
#define BACKLOG 10
#define BUFFER_CAPACITY 16000

// https://en.wikipedia.org/wiki/C_data_types
// COMPILE + RUN: make -C build && ./build/webserver 

// function declarations:
void handle_http_request(char buffer[]);
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
    hints.ai_family = AF_UNSPEC; //automatic IPv4 /IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(port_in[1], port_in[2], &hints, &res); //writes into res

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); // get socket file descriptor

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    bind(sockfd, res->ai_addr, res->ai_addrlen); // bind to specific computer port
    listen(sockfd, BACKLOG);

    addr_size = sizeof storage;

    int output_valid_bytes = 0;
    char output[BUFFER_CAPACITY];
    memset(output, 0, sizeof(output)); // setzt \0 überall

    while(1) {
        new_fd = accept(sockfd, (struct sockaddr *)&storage, &addr_size);

        while(1) {
            int bytes_send = recv(new_fd, output + output_valid_bytes * sizeof(char), sizeof(output), 0); //can have multiple receives if == 0 geschlossen, < 0 Fehler, > 0 wurden Bytes empfangen
            printf("one received\n");
            if(bytes_send < 0) {
                printf("ERROR:PACKET_RECEIVE_PROBLEM\n");
                break;
            } else if (bytes_send == 0) {
                memset(output,0,sizeof(output));
                printf("end of transmission reached\n");
                break;
            }
            output_valid_bytes = output_valid_bytes + bytes_send;
            if(bytes_send > 0) {
                output[output_valid_bytes] = '\0';
            }

            while(1) { //check for complete http requests
                char* request_end = strstr(output, "\r\n\r\n");
                if(request_end != NULL) { // one message send completly
                    handle_http_request(output);
                    send(new_fd,"Reply\r\n\r\n",9,0);
                    clear_one_request_buffer(output, request_end, &output_valid_bytes);
                } else {
                    break;
                }
            } 
        }


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



