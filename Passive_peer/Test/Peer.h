#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctime>

#include "MyOpenCV.h"


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_ADDR "localhost"



int do_receive(int sockfd, void *buf, size_t len, int flags);

int do_send(int sockfd, void* msg, size_t len, int flags);

int do_receive_image(int sockfd);

int do_send_image(int sockfd, const char* filename);

int do_send_mat(int sockfd, Mat & frame);

int do_receive_mat(int sockfd, Mat & frame);

void zero_info(struct addrinfo* hints, struct addrinfo* result);