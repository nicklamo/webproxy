#include "webproxy.h"
/***********************************************************************************************************/
/*********************** MAIN - init threads, build server, accept incomming request **********************/
/***********************************************************************************************************/
int main(int argc, char *argv[]) {
    // define variables
    int sockfd, optval, port, status, connection;
    FILE * fp;
    char line[LINE_SIZE];

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    pthread_t server_threads[MAX_CONNECTIONS];
    pthread_t  client_threads[MAX_CONNECTIONS];
    // read command line arg
    if (argc != 3) err(1, "usage: ./webproxy <port-number> <time-to-live>");

    // create and init shared data
    struct global_data data;
    pthread_mutex_init(&data.buff_m, NULL);
    pthread_mutex_init(&data.client_conns_m, NULL);
    pthread_mutex_init(&data.cache_m, NULL);
    pthread_cond_init(&data.client, NULL);
    pthread_cond_init(&data.cons, NULL);
    pthread_cond_init(&data.prod, NULL);

    data.read_pos = 0;
    data.write_pos = 0;
    data.curr_size = 0;
    data.client_conn_idx = 0;
    data.num_conns = 0;
    data.ttl = atoi(argv[2]);
    for (int i =0; i < MAX_CONNECTIONS; i++) { 
        data.client_conns[i] = -1; 
    }
    // init cache to null
    for (int i=0; i <CACHE_SIZE; i++) {
        data.cache[i] = NULL;
    }
    // init blacklist
    fp = fopen("blacklist.txt", "r");
    int i=0;
    while(fgets(line, LINE_SIZE, fp)){
        data.blacklist[i] = line;
        i++;
    }
    data.bl_len = i;
    fclose(fp);
    printf("Global data created\n");

    /* create threads that will make requests to servers */
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        pthread_create( &server_threads[i],  NULL, server, &data);
    }
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        pthread_create( &client_threads[i],  NULL, client, &data);
    }
    printf("Threads created\n");
    // build server
    port = atoi(argv[1]);
    sockfd = build_server(port); 
    
    // loop forever
    for ( ; ; ) {
        // accept incoming requests
        bzero((char *) &clientaddr, sizeof(clientaddr));
        if ((connection = accept(sockfd, (struct sockaddr *) &clientaddr, &clientlen)) < 0) err(1, "Error making connection \n");
        
        // put new connection into client shared array
        pthread_mutex_lock(&data.client_conns_m);
        data.num_conns++;
        data.client_conns[data.client_conn_idx] = connection;
        pthread_cond_signal(&data.client);
        pthread_mutex_unlock(&data.client_conns_m);

        printf("\nRequest from %s:%d \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    }
}

/***********************************************************************************/
/********************** Build the server based on port number ***********************/
/***********************************************************************************/
int build_server(int port_num) {
    // define variables
    int sockfd, optval, connection, status;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t clientlen;

    /* create parent socket */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) err(1, "Error opening socket\n");
    

    // setsockopt: configure socket to reuse address
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    // build server 
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port_num);

    // bind socket
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) err(1, "Error binding socket. error is:\n");

    // listen for requests
    if (listen(sockfd, MAX_CONNECTIONS) < 0) err(1 ,"Error in listening.\n"); 
    printf("Listening... \n");
    return sockfd;
}