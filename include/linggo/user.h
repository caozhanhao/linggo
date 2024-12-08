#ifndef LINGGO_USER_H
#define LINGGO_USER_H
#pragma once

#include "error.h"
#include "json-parser/json.h"

#include <stdio.h>

#define LINGGO_GUEST_USERNAME "__linggo_guest__"
#define LINGGO_GUEST_PASSWORD "__linggo_guest__"

typedef struct linggo_user
{
    int uid;
    char* name;
    char* passwd;
    struct linggo_user* next;
} linggo_user;

typedef struct
{
    linggo_user* db;
    int next_uid;
} linggo_user_database;

extern linggo_user_database linggo_usrdb;

enum LINGGO_CODE linggo_user_init();
enum LINGGO_CODE linggo_user_register(char* username, char* password);
enum LINGGO_CODE linggo_user_login(char* username, char* password, linggo_user** user);
enum LINGGO_CODE linggo_user_get_quiz(linggo_user* user, size_t idx, json_value** quiz);
#endif