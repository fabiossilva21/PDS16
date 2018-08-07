#define _GNU_SOURCE
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
#include <pthread.h>
#include <signal.h>

int previousState = -1;
int cpid;

bool toEnd = false;
bool breakpointHitG = false;

char sendBuff[1024] = {0};
char recvBuff[1024] = {0};
char message[1024] = {0};

pthread_t tsids[2];

int server_socket = 0, client_socket = 0, clilen = 0;
struct sockaddr_in server_addr, client_addr;
