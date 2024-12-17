#include "linggo/error.h"

const char* linggo_strerror(enum LINGGO_CODE code)
{
    switch (code)
    {
    case LINGGO_OK:
        return "OK";
    case LINGGO_INVALID_CONFIG_PATH:
        return "Invalid configuration path";
    case LINGGO_INVALID_CONFIG:
        return "Invalid configuration format";
    case LINGGO_INVALID_VOC_PATH:
        return "Invalid vocabulary path";
    case LINGGO_INVALID_VOC:
        return "Invalid vocabulary format";
    case LINGGO_INVALID_DB_PATH:
        return "Invalid database path";
    case LINGGO_INVALID_DB:
        return "Invalid database format";
    case LINGGO_OUT_OF_MEMORY:
        return "Out of memory";
    case LINGGO_UNEXPECTED_ERROR:
        return "Unexpected error";
    case LINGGO_USER_NOT_FOUND:
        return "User not found";
    case LINGGO_WRONG_PASSWORD:
        return "Wrong password";
    case LINGGO_INVALID_USER:
        return "Invalid user";
    case LINGGO_AIGEN_QUIZ_ERROR:
        return "Failed to generate quiz";
    }
}
