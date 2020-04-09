#include "threads.h"

/***********************************************************************************************************/
/******** Code for threads that handle client requests, checks cache, or puts in shared buff ***************/
/***********************************************************************************************************/
void *client(void *arg){
    char file[LINE_SIZE], protocol[LINE_SIZE], command[LINE_SIZE];
    int curr_conn, hash;
    int n;
    char buffer[REQUEST_SIZE], res[REQUEST_SIZE];
    struct buff_entry *entry;
    struct global_data *data = (struct global_data*) arg;
    
    while (1) {
        /* Get a request to service */
        pthread_mutex_lock(&data->client_conns_m);
        while( data->num_conns <= 0) { // block if no connections to handle
            pthread_cond_wait(&data->client, &data->client_conns_m);
        }
        curr_conn = data->client_conns[data->client_conn_idx];
        data->client_conns[data->client_conn_idx] = -1;
        data->num_conns--;
        data->client_conn_idx = (data->client_conn_idx + 1 ) % MAX_CONNECTIONS;
        pthread_mutex_unlock(&data->client_conns_m);

        /* Handle this request */
        // recive data
        n=recv(curr_conn, buffer, REQUEST_SIZE, 0);
        
        sscanf(buffer, "%s %s %s", command, file, protocol);

        /* Can Only service GET*/
        if (strcmp("GET", command)){
            printf("not a GET \n");
            char err_content_header[LINE_SIZE];
            sprintf(err_content_header, "%s 400 Bad Request\r\n\r\n", protocol);
            send(curr_conn, err_content_header, strlen(err_content_header), 0);
            close(curr_conn);
            continue;
        }
        // format url for blacklist check
        if (file[strlen(file)-1] == '/'){
            file[strlen(file)-1] = '\0';
        }
        /* check blacklist */
        if (is_blacklisted(file, data->blacklist, data->bl_len) ){
            printf("request has been blacklisted \n");
            char err_content_header[LINE_SIZE];
            sprintf(err_content_header, "%s 403 Forbidden\r\n\r\n", protocol);
            send(curr_conn, err_content_header, strlen(err_content_header), 0);
            close(curr_conn);
            continue;
        }
        /*  check cache */
        hash = hash_url(file);
        if (hash != -1 && is_cached(hash, data->cache, data->ttl)){
            printf("Servicing request with cache \n");
            send(curr_conn, data->cache[hash]->entire_request, strlen(data->cache[hash]->entire_request), 0);
            close(curr_conn);
            continue;
        }
        printf("Request not cached\n");

        /* put request into shared buffer for server threads to handle */
        struct buff_entry *entry = malloc(sizeof(struct buff_entry));
        strcpy(entry->request, buffer);
        entry->client_socket = curr_conn;

        pthread_mutex_lock(&data->buff_m);
        while (data->curr_size >= BUFFER_SIZE) { // block if buffer is full
            pthread_cond_wait(&data->prod, &data->buff_m);
        }
        // put request into buffer
        data->buff[data->write_pos] = entry;

        // update shared data
        data->curr_size++;
        data->write_pos = (data->write_pos+1) % BUFFER_SIZE;
        pthread_cond_signal(&data->cons);
        pthread_mutex_unlock(&data->buff_m);
    }
    pthread_exit(NULL);
}

/***********************************************************************************************************/
/*************** Code for threads that make request to server based on data in shared buff *****************/
/***********************************************************************************************************/
void *server(void *arg){
    char url[LINE_SIZE];
    int client_conn, serv_conn, optval, n, hash;
    char buffer[REQUEST_SIZE], res[MAX_FILE_SIZE], final_res[MAX_FILE_SIZE];
    char protocol[LINE_SIZE], status[LINE_SIZE], command[LINE_SIZE];
    struct buff_entry *entry;
    struct global_data *data = (struct global_data*) arg;

    while(1) {
        /* Get a request to service */
        pthread_mutex_lock(&data->buff_m);
        while(data->curr_size <=0){ // block if nothing in shared buff
            pthread_cond_wait(&data->cons, &data->buff_m);
        }
        // get an entry from shared buffer
        entry = data->buff[data->read_pos];
        // parse data
        client_conn = entry->client_socket;
        strcpy(buffer, entry->request);
        // update shared data
        data->curr_size--;
        data->read_pos = (data->read_pos+1) % BUFFER_SIZE;
        // free request from buffer
        free(entry);
        pthread_cond_signal(&data->prod);
        pthread_mutex_unlock(&data->buff_m);

        /* send out request to server */
        // create socket 
        if ( (serv_conn = socket(AF_INET, SOCK_STREAM, 0)) < 0) err(1, "Error opening socket\n");
        optval = 1;
        setsockopt(serv_conn, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
        // send request
        if (n = send_request(buffer, serv_conn) == 0 ) {
            printf("Error Sending request to server\n");
            char err_content_header[LINE_SIZE];
            sprintf(err_content_header, "%s 400 Bad Request\r\n\r\n", protocol);
            send(client_conn, err_content_header, strlen(err_content_header), 0);
            close(serv_conn);
            close(client_conn);
            continue;
        }

        bzero(res, MAX_FILE_SIZE);

        /* forward data recieved to client */
        while(n = recv(serv_conn , res, MAX_FILE_SIZE, 0) > 0 ){
            send(client_conn, res, strlen(res), 0);
            strcat(final_res, res);
            bzero(res, MAX_FILE_SIZE);
        }
        // close connections
        close(serv_conn);
        close(client_conn);

        /* Cache the response*/
        sscanf(buffer, "%s %s %s", command, url, protocol);
        hash = hash_url(url);
        printf("Caching %s at hash: %d\n", url, hash);
        cache(hash, final_res, data->cache);
    }
    pthread_exit(NULL);
}

/***********************************************************************************************************/
/************************** Create and send request to a server (serv_conn) ********************************/
/***********************************************************************************************************/
int send_request(char *buffer, int serv_conn) {
    int n;
    char final_request[REQUEST_SIZE], protocol[LINE_SIZE], command[LINE_SIZE], url[LINE_SIZE],
          final_url[LINE_SIZE], page[LINE_SIZE], *tmp;
    struct hostent *server;
    struct sockaddr_in addr;
    struct timeval  timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* parse data */
    sscanf(buffer, "%s %s %s", command, url, protocol);
    /* parse url */
    int port = 80;
    int i = strlen(url)-1;
    while(url[i] != ':' && i != 0) {i--;} // check if there is a colon anywhere

    if (i == 4) { // no port num but http://
        sscanf(url, "http://%99[^/]/%99[^\n]", final_url, page);
    }
    else if (i==0){ // no port or http://
        sscanf(url, "%99[^/]/%99[^\n]", final_url, page);
    }
    else if (i > 4 && url[4] != ':') { // port no http://
        sscanf(url, "%99[^:]:%99d/%99[^\n]", final_url, &port, page);
    }
    else { // port num and http://
        sscanf(url, "http://%99[^:]:%99d/%99[^\n]", final_url, &port, page);
    }
    
    tmp = strdup(page);
    strcpy(page, "/");
    strcat(page, tmp);
    free(tmp);

    printf("URL = %s, Port number = %d, Page = %s \n", final_url, port, page);
    
    // get server data
    if ((server = gethostbyname(final_url)) == NULL) {
        printf("Bad hostname: %s\n", final_url);
        return 0;
    }
    /* build the server's internet address */
    // zero out addr struct
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);
    addr.sin_port = htons(port);
    
    fd_set set;
    FD_ZERO(&set);
    FD_SET(serv_conn, &set);
    // connect to server
    fcntl(serv_conn, F_SETFL, O_NONBLOCK);
    if ( (n= connect(serv_conn, (struct sockaddr *) &addr, sizeof(addr))) == -1){
        if (errno != EINPROGRESS) {
            return 0;
        }
    }
    select(serv_conn+1, NULL, &set, NULL, &timeout);
    fcntl(serv_conn, F_SETFL, fcntl(serv_conn, F_GETFL, 0) & ~O_NONBLOCK);
    // send request
    sprintf(final_request, "%s %s %s \r\nHost: %s\r\n\r\n", command, page, protocol, final_url);
    send(serv_conn, final_request, REQUEST_SIZE, 0);
    return 1;
}