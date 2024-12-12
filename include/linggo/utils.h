#ifndef LINGGO_UTILS_H
#define LINGGO_UTILS_H
#pragma once

#include "json-parser/json.h"

json_value* linggo_json_find_key(json_value* obj, const char* key);

int linggo_starts_with(const char* str1, const char* str2);

typedef struct
{
    char* key;
    char* value;
} http_request_param;

typedef struct
{
    http_request_param* params;
    size_t size;
} http_request_params;

http_request_params linggo_parse_params(char* path);
char* linggo_http_find_key(http_request_params params, const char* key);
void linggo_free_params(http_request_params params);

void linggo_shuffle(void *base, size_t nitems, size_t size);
#endif