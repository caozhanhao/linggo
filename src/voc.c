#include "linggo/voc.h"
#include "linggo/error.h"
#include "linggo/utils.h"

#include "json-parser/json.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

linggo_vocabulary linggo_voc;

enum LINGGO_CODE linggo_voc_init(const char* voc_path)
{
    FILE* fp = fopen(voc_path, "r");
    if (fp == NULL)
        return LINGGO_INVALID_VOC_PATH;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = malloc(size + 1);

    if (buffer == NULL)
        return LINGGO_OUT_OF_MEMORY;

    fread(buffer, 1, size, fp);
    fclose(fp);
    buffer[size] = '\0';

    json_value* json = json_parse(buffer, size);

    if (json == NULL || json->type != json_array) return LINGGO_INVALID_VOC;

    linggo_voc.voc_size = json->u.array.length;
    linggo_voc.lookup_table = malloc(sizeof(linggo_voc) * linggo_voc.voc_size);
    memset(linggo_voc.lookup_table, 0, sizeof(linggo_voc) * linggo_voc.voc_size);

    for (int i = 0; i < linggo_voc.voc_size; ++i)
    {
        json_value* jword = linggo_json_find_key(json->u.array.values[i], "word");
        json_value* jmeaning = linggo_json_find_key(json->u.array.values[i], "meaning");
        json_value* jexplanation = linggo_json_find_key(json->u.array.values[i], "explanation");
        if (jword == NULL || jword->type != json_string
            || jmeaning == NULL || jmeaning->type != json_string
            || jexplanation == NULL || jexplanation->type != json_string)
            return LINGGO_INVALID_VOC;

        linggo_voc.lookup_table[i] = (linggo_word){
            jword->u.string.ptr,
            jmeaning->u.string.ptr,
            jexplanation->u.string.ptr
        };
    }

    free(buffer);
    return LINGGO_OK;
}

void linggo_voc_free()
{
    free(linggo_voc.lookup_table);
    json_value_free(linggo_voc.vocabulary);
}

typedef struct search_node
{
    uint32_t data;
    struct search_node* next;
} search_node;

typedef struct hash_node
{
    uint32_t data;
    const char* raw;
    struct hash_node* next;
} hash_node;

int hash(const char* key)
{
    int h = 0;
    size_t len = strlen(key);
    for (int i = 0; i < len; ++i)
        h = 33 * h + key[i];
    return h;
}

static hash_node** diffs;
static uint32_t diffs_len;

int64_t get_diff(const char* key)
{
    hash_node* h = diffs[hash(key) % diffs_len];
    while (h != NULL)
    {
        if (strcmp(h->raw, key) == 0)
            return h->data;
        h = h->next;
    }
    return -1;
}

int cmp_func(const void * a, const void* b)
{
    const char* astr = linggo_voc.lookup_table[*(uint32_t*)a].word;
    const char* bstr = linggo_voc.lookup_table[*(uint32_t*)b].word;
    int64_t ad = get_diff(astr);
    int64_t bd = get_diff(bstr);
    assert(ad != -1 && bd != -1);
    return ad > bd;
}

linggo_voc_search_result linggo_voc_search(const char* target)
{
    search_node* head = malloc(sizeof(search_node));
    memset(head, 0, sizeof(search_node));

    search_node* curr = head;

    size_t target_len = strlen(target);
    size_t len = 0;
    for (int i = 0; i < linggo_voc.voc_size; ++i)
    {
        if (strcmp(target, linggo_voc.lookup_table[i].word) == 0
            || strstr(target, linggo_voc.lookup_table[i].meaning) != NULL
            || linggo_get_edit_distance(target, linggo_voc.lookup_table[i].word) < target_len / 2
            || linggo_starts_with(linggo_voc.lookup_table[i].word, target))
        {
            curr->data = i;
            curr->next = malloc(sizeof(search_node));
            curr = curr->next;
            memset(curr, 0, sizeof(search_node));
            ++len;
        }
    }
    if (len == 0)
        return (linggo_voc_search_result){NULL, 0};

    // set up result
    linggo_voc_search_result ret;
    ret.data = malloc(sizeof(uint32_t) * len);
    ret.size = len;

    // setup diffs
    diffs = malloc(sizeof(hash_node) * len);
    diffs_len = len;
    memset(diffs, 0, sizeof(hash_node) * len);

    int retpos = 0;
    curr = head;
    while (curr != NULL)
    {
        const char* word = linggo_voc.lookup_table[curr->data].word;
        int h = hash(word) % diffs_len;

        hash_node** a = &diffs[h];
        while (*a != NULL)
            a = &(*a)->next;
        *a = malloc(sizeof(hash_node));
        (*a)->data = linggo_get_edit_distance(target, word);
        (*a)->raw = word;
        (*a)->next = NULL;

        ret.data[retpos++] = curr->data;

        curr = curr->next;
    }

    qsort(ret.data, len, sizeof(uint32_t), cmp_func);

    // cleanup
    for (int i = 0; i < len; ++i)
    {
        if (diffs[i] != NULL)
        {
            hash_node* a = diffs[i];
            hash_node* next;
            while (a != NULL)
            {
                next = a->next;
                free(a);
                a = next;
            }
        }
    }
    free(diffs);
    diffs = NULL;

    curr = head;
    search_node* next;
    while (curr != NULL)
    {
        next = curr->next;
        free(curr);
        curr = next;
    }
    head = NULL;
    curr = NULL;

    return ret;
}

void linggo_voc_search_free(linggo_voc_search_result result)
{
    free(result.data);
}
