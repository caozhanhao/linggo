#ifndef LINGGO_SERVER_H
#define LINGGO_SERVER_H
#pragma once

#include "error.h"

struct linggo_server_context
{
    char* resource_path;
    char* admin_password;
    char* listen_address;
    int listen_port;
    char* smtp_server;
    char* smtp_username;
    char* smtp_password;
    char* smtp_email;
};

extern struct linggo_server_context linggo_svrctx;

enum LINGGO_CODE linggo_init(const char* config_path);

enum LINGGO_CODE linggo_server_start();

#endif