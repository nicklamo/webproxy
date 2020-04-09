#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "webproxy.h"

#ifndef HELPERS_H
#define HELPERS_H

int hash_url(char *name);
int is_cached(int hash, struct cache_entry* cache[], int ttl);
void cache(int hash, char *res, struct cache_entry* cache[]);
int is_blacklisted(char *url, char *blacklist[], int bl_len);

#endif 