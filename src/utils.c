#include "linggo/utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

json_value* linggo_json_find_key(json_value* obj, const char* key)
{
    if (obj == NULL || obj->type != json_object)
        return NULL;
    for (int i = 0; i < obj->u.object.length; i++)
    {
        if (strcmp(obj->u.object.values[i].name, key) == 0)
            return obj->u.object.values[i].value;
    }
    return NULL;
}

int linggo_starts_with(const char* str1, const char* str2)
{
    size_t len2 = strlen(str2);
    for (int i = 0; i < len2; i++)
    {
        if (str1[i] != str2[i] || str1[i] == '\0')
            return 0;
    }
    return 1;
}

http_request_params linggo_parse_params(char* path)
{
    int i = 0;
    while (path[i] != '\0' && path[i] != '?') ++i;
    if (path[i] == '\0')
        return (http_request_params){NULL, 0};

    int params_cnt = 0;
    for (int scan = i; path[scan] != '\0'; ++scan)
    {
        if (path[scan] == '?')
        {
            if (params_cnt != 0)
                return (http_request_params){NULL, 0};
            params_cnt = 1;
        }
        else if (path[scan] == '&')
            ++params_cnt;
    }

    http_request_param* params = malloc(sizeof(http_request_param) * params_cnt);
    memset(params, 0, sizeof(http_request_param) * params_cnt);

    int param_pos = 0;
    int eq = -1;
    int last = i;
    int j = i;
    for (; path[j] != '\0'; ++j)
    {
        if (path[j] == '=')
            eq = j;
        else if (path[j] == '&')
        {
            if (eq == -1)
            {
                linggo_free_params((http_request_params){params, param_pos});
                return (http_request_params){NULL, 0};
            }
            char* key = malloc(eq - last);
            strncpy(key, path + last + 1, eq - last - 1);
            key[eq - last - 1] = '\0';
            char* value = malloc(j - eq);
            strncpy(value, path + eq + 1, j - eq - 1);
            value[j - eq - 1] = '\0';

            params[param_pos].key = key;
            params[param_pos].value = value;
            ++param_pos;
            last = j;
            eq = -1;
        }
    }

    if (eq != -1)
    {
        char* key = malloc(eq - last);
        strncpy(key, path + last + 1, eq - last - 1);
        key[eq - last - 1] = '\0';
        char* value = malloc(j - eq);
        strncpy(value, path + eq + 1, j - eq - 1);
        value[j - eq - 1] = '\0';

        params[param_pos].key = key;
        params[param_pos].value = value;
        ++param_pos;
    }

    if (params_cnt != param_pos)
    {
        linggo_free_params((http_request_params){params, param_pos});
        return (http_request_params){NULL, 0};
    }
    return (http_request_params){params, params_cnt};
}

char* linggo_http_find_key(http_request_params params, const char* key)
{
    if (params.params == NULL) return NULL;
    for (int i = 0; i < params.size; i++)
    {
        if (strcmp(params.params[i].key, key) == 0)
            return params.params[i].value;
    }
    return NULL;
}

void linggo_free_params(http_request_params params)
{
    for (int i = 0; i < params.size; i++)
    {
        free(params.params[i].key);
        free(params.params[i].value);
    }
    free(params.params);
}

void linggo_shuffle(void* base, size_t nitems, size_t size)
{
    if (nitems == 0 || size == 0) return;
    char temp[size];
    for (size_t i = nitems - 1; i >= 1; --i)
    {
        size_t r = rand() % (i + 1);
        memmove(temp, base + r * size, size);
        memmove(base + r * size, base + i * size, size);
        memmove(base + i * size, temp, size);
    }
}

int linggo_get_edit_distance(const char* s1, const char* s2)
{
    size_t n = strlen(s1);
    size_t m = strlen(s2);
    if (n * m == 0) return (int)(n + m);
    int D[n + 1][m + 1];
    for (int i = 0; i < n + 1; i++)
    {
        D[i][0] = i;
    }
    for (int j = 0; j < m + 1; j++)
    {
        D[0][j] = j;
    }

    for (int i = 1; i < n + 1; i++)
    {
        for (int j = 1; j < m + 1; j++)
        {
            int left = D[i - 1][j] + 1;
            int down = D[i][j - 1] + 1;
            int left_down = D[i - 1][j - 1];
            if (s1[i - 1] != s2[j - 1]) left_down += 1;
#define linggo_min(a,b) (((a) < (b)) ? (a) : (b))
            D[i][j] = linggo_min(left, linggo_min(down, left_down));
#undef linggo_min
        }
    }
    return D[n][m];
}