#ifndef LINGGO_VOC_H
#define LINGGO_VOC_H
#pragma once
#include "json-parser/json.h"

typedef struct
{
    char* word;
    char* meaning;
    char* explanation;
} linggo_word;

typedef struct
{
    json_value* vocabulary;
    linggo_word* lookup_table;
    size_t voc_size;
} linggo_vocabulary;

extern linggo_vocabulary linggo_voc;

enum LINGGO_CODE linggo_voc_init(const char* voc_path);

void linggo_voc_free();

typedef struct
{
    size_t* data;
    size_t size;
} linggo_voc_search_results;

linggo_voc_search_results linggo_voc_search(const char* word);

void linggo_voc_search_free(linggo_voc_search_results res);


#endif