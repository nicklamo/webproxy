#ifndef HANDLERS
#define HANDLERS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <err.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>

#include "webproxy.h"
#include "helpers.h"

/* define functions and globals */
#define h_addr h_addr_list[0]
#define LINE_SIZE 256

void *client(void *arg);
void *server(void *arg);
int send_request(char *buffer, int serv_conn);

#endif