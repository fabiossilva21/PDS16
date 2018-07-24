#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "logging.h"

int previousState = -1;

char sendBuff[1024] = {0};
char recvBuff[1024] = {0};
char message[1024] = {0};

int server_socket = 0, client_socket = 0, clilen = 0;
struct sockaddr_in server_addr, client_addr;
