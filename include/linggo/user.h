#ifndef LINGGO_USER_H
#define LINGGO_USER_H
#pragma once

#include "error.h"
#include "json-parser/json.h"

#include <stdio.h>

#define LINGGO_GUEST_USERNAME "__linggo_guest__"
#define LINGGO_GUEST_PASSWORD "__linggo_guest__"

typedef struct linggo_marked_word
{
    size_t idx;
    struct linggo_marked_word* next;
} linggo_marked_word;

typedef struct linggo_user
{
    int uid;
    char* name;
    char* passwd;
    size_t curr_memorize_word;
    linggo_marked_word* marked_words;
    struct linggo_user* next;
} linggo_user;

typedef struct
{
    linggo_user* db;
    int next_uid;
} linggo_user_database;

extern linggo_user_database linggo_usrdb;

enum LINGGO_CODE linggo_userdb_init();
void linggo_userdb_free();

enum LINGGO_CODE linggo_user_register(const char* username, const char* password);
enum LINGGO_CODE linggo_user_login(const char* username, const char* password, linggo_user** user);
enum LINGGO_CODE linggo_user_get_quiz(linggo_user* user, size_t idx, json_value** quiz);
enum LINGGO_CODE linggo_user_mark_word(linggo_user* user, size_t idx);
enum LINGGO_CODE linggo_user_unmark_word(linggo_user* user, size_t idx);
int linggo_user_is_marked_word(linggo_user* user, size_t idx);
#endif