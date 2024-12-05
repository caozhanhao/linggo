#ifndef LINGGO_VOC_H
#define LINGGO_VOC_H
#pragma once
#include "json-parser/json.h"

struct linggo_word
{
    const char* word;
    const char* meaning;
    const json_value* group;
};

struct linggo_word_node
{
    struct linggo_word* word;
    struct linggo_word_node* next;
};

struct linggo_vocabulary
{
    json_value* vocabulary;
    struct linggo_word_node* index;
} linggo_voc;

#endif