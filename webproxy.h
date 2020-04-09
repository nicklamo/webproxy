#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <err.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>

#include "threads.h"

#ifndef WEBPROXY_H
#define WEBPROXY_H

/* define globals and functions, structs for threads*/
#define MAX_CONNECTIONS 25
#define BUFFER_SIZE 100
#define CACHE_SIZE 500
#define REQUEST_SIZE 4096
#define MAX_FILE_SIZE 65535

int build_server(int port_num);

struct buff_entry {
    char request[REQUEST_SIZE];
    int client_socket;
};

struct cache_entry {
    char entire_request[MAX_FILE_SIZE];
    time_t time_in;
};

struct global_data {
    /* data shared by client servicing threads */
    int client_conns[MAX_CONNECTIONS];
    int num_conns;
    int client_conn_idx;
    /* data shared by all threads */
    struct buff_entry *buff[BUFFER_SIZE];
    int read_pos;
    int write_pos;
    int curr_size;
    struct cache_entry *cache[CACHE_SIZE];
    int ttl;
    char *blacklist[50];
    int bl_len;
    // pthreads stuff
    pthread_mutex_t buff_m;
    pthread_mutex_t client_conns_m;
    pthread_mutex_t cache_m;
    pthread_cond_t cons;
    pthread_cond_t client;
    pthread_cond_t prod;
};

#endif