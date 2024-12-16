#ifndef LINGGO_SERVER_H
#define LINGGO_SERVER_H
#pragma once

#include "error.h"

typedef struct
{
    char* resource_path;
    char* admin_password;
    char* listen_address;
    char* version;
    int listen_port;
} linggo_server_context;

extern linggo_server_context linggo_svrctx;

enum LINGGO_CODE linggo_server_init(const char* config_path);

void linggo_server_free();

enum LINGGO_CODE linggo_server_start();

#endif
