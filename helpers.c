#include "helpers.h"

int hash_url(char *name){
    /* gen hash based on name and return */
    //make sure url is hashable
    for (int i = 0; i< strlen(name)-1; i++){
        if (name[i] == '?'){
            return -1;
        }
    }
    // by djb2 on stack overflow
    unsigned long hash = 5381;
    int c;
    while (c = *name++)
        hash = ((hash << 5) + hash) + c;
    return hash % CACHE_SIZE;
}

int is_cached(int hash, struct cache_entry* cache[], int ttl){
    time_t curr_time; 
    if (cache[hash] ==NULL) {
        return 0;
    }
    //check if expire
    time(&curr_time);
    if (cache[hash]->time_in+60 < curr_time){
        printf("TimeStamp expired: %ld / %ld\n", cache[hash]->time_in+60 , curr_time);
        return 0;
    }
    // check if there is anything in there
    if (strlen(cache[hash]->entire_request) > 0){
        printf("Found the data \n");
        return 1;
    }
    else {
        return 0;
    }
}

void cache(int hash, char *res, struct cache_entry* cache[]){
    if (hash == -1){
        return;
    }
    // place res based on hash generated
    if (cache[hash] == NULL) {
        cache[hash] = malloc(sizeof(struct cache_entry));
    }
    time(&cache[hash]->time_in);
    // timestamp
    strcpy(cache[hash]->entire_request, res);
}

int is_blacklisted(char *url, char *blacklist[], int bl_len){
    // check if url is blacklisted
    int i;
    for (i=0; i < bl_len; i++){
        if (!strcmp(blacklist[i], url)){
            return 1;
        }
    }
    return 0;
}