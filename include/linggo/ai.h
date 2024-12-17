#ifndef LINGGO_AI_H
#define LINGGO_AI_H
#pragma once

#include "error.h"

#include <stdint.h>

typedef struct
{
    char* question;
    char* options[4];
    uint8_t answer;
} linggo_ai_quiz;

enum LINGGO_CODE linggo_ai_gen_quiz(const char* word, linggo_ai_quiz* quiz);

void linggo_ai_quiz_free(linggo_ai_quiz quiz);

#endif
