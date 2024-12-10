#include "linggo/user.h"
#include "linggo/error.h"

#include "json-builder/json-builder.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linggo/voc.h>
linggo_user_database linggo_usrdb;

enum LINGGO_CODE linggo_user_init()
{
    linggo_usrdb.db = NULL;
    linggo_usrdb.next_uid = 0;
    linggo_user_register(LINGGO_GUEST_USERNAME, LINGGO_GUEST_PASSWORD);
    return LINGGO_OK;
}

enum LINGGO_CODE linggo_user_register(char* username, char* password)
{
    size_t namelen = strlen(username);
    size_t passwdlen = strlen(password);

    linggo_user** curr = &linggo_usrdb.db;
    while (*curr != NULL)
        *curr = (*curr)->next;
    *curr = malloc(sizeof(linggo_user));
    memset(*curr, 0, sizeof(linggo_user));
    (*curr)->uid = linggo_usrdb.next_uid++;
    (*curr)->name = malloc(namelen + 1);
    memcpy((*curr)->name, username, namelen);
    (*curr)->name[namelen] = '\0';
    (*curr)->passwd = malloc(passwdlen + 1);
    memcpy((*curr)->passwd, password, passwdlen);
    (*curr)->passwd[passwdlen] = '\0';
    (*curr)->next = NULL;
    return LINGGO_OK;
}

enum LINGGO_CODE linggo_user_login(char* username, char* password, linggo_user** user)
{
    linggo_user** curr = &linggo_usrdb.db;
    while (*curr != NULL)
    {
        if (strcmp((*curr)->name, username) == 0)
        {
            if (strcmp((*curr)->passwd, password) == 0)
            {
                if (user != NULL)
                    *user = *curr;
                return LINGGO_OK;
            }
            else
                return LINGGO_WRONG_PASSWORD;
        }
        *curr = (*curr)->next;
    }
    return LINGGO_USER_NOT_FOUND;
}

enum LINGGO_CODE linggo_user_mark_word(linggo_user* user, size_t idx)
{
    if (user == NULL) return LINGGO_INVALID_PARAM;
    linggo_marked_word** curr = &user->marked_words;
    while (*curr != NULL)
        *curr = (*curr)->next;
    *curr = malloc(sizeof(linggo_marked_word));
    memset(*curr, 0, sizeof(linggo_marked_word));
    (*curr)->idx = idx;
    (*curr)->next = NULL;
    return LINGGO_OK;
}

int linggo_user_is_marked_word(linggo_user* user, size_t idx)
{
    if (user == NULL) return -1;
    linggo_marked_word** curr = &user->marked_words;
    while (*curr != NULL)
    {
        if ((*curr)->idx == idx)
            return 1;
        *curr = (*curr)->next;
    }
    return 0;
}

enum LINGGO_CODE linggo_user_get_quiz(linggo_user* user, size_t idx, json_value** quiz)
{
    if (user == NULL) return LINGGO_INVALID_PARAM;
    size_t word0, word1, word2;

    srand(time(NULL));
    word0 = rand() % linggo_voc.voc_size;
    word1 = rand() % linggo_voc.voc_size;
    word2 = rand() % linggo_voc.voc_size;

    const char* opt[4] = { "A", "B", "C", "D" };
    json_value* json = json_object_new(4);
    json_value* options = json_object_new(4);
    static int flag = 1;
    if (flag)
    {
        flag = 0;
        json_object_push(json, "question", json_string_new(linggo_voc.lookup_table[idx].word));
        json_object_push(options, opt[0], json_string_new(linggo_voc.lookup_table[word0].meaning));
        json_object_push(options, opt[1], json_string_new(linggo_voc.lookup_table[word1].meaning));
        json_object_push(options, opt[2], json_string_new(linggo_voc.lookup_table[word2].meaning));
        json_object_push(options, opt[3], json_string_new(linggo_voc.lookup_table[idx].meaning));
        json_object_push(json, "options", options);
    }
    else
    {
        flag = 1;
        json_object_push(json, "question", json_string_new(linggo_voc.lookup_table[idx].meaning));
        json_object_push(options, opt[0], json_string_new(linggo_voc.lookup_table[word0].word));
        json_object_push(options, opt[1], json_string_new(linggo_voc.lookup_table[word1].word));
        json_object_push(options, opt[2], json_string_new(linggo_voc.lookup_table[word2].word));
        json_object_push(options, opt[3], json_string_new(linggo_voc.lookup_table[idx].word));
        json_object_push(json, "options", options);
    }

    json_value* indexes = json_object_new(4);
    json_object_push(indexes, opt[0], json_integer_new(word0));
    json_object_push(indexes, opt[1], json_integer_new(word1));
    json_object_push(indexes, opt[2], json_integer_new(word2));
    json_object_push(indexes, opt[3], json_integer_new(idx));
    json_object_push(json, "indexes", indexes);

    json_object_push(json, "answer", json_string_new(opt[3]));
    *quiz = json;
    return LINGGO_OK;
}