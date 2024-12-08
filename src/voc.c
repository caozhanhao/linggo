#include "linggo/voc.h"
#include "linggo/error.h"
#include "linggo/utils.h"

#include "json-parser/json.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

linggo_vocabulary linggo_voc;

enum LINGGO_CODE linggo_voc_init(const char* voc_path)
{
    int fd = open(voc_path, O_RDONLY);
    struct stat statbuf;
    if (fd < 0 || stat(voc_path, &statbuf) != 0) return LINGGO_INVALID_VOC_PATH;
    char* pvoc = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pvoc == MAP_FAILED) return LINGGO_INVALID_VOC_PATH;
    json_value* json = json_parse(pvoc, statbuf.st_size);

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

        linggo_voc.lookup_table[i] = (linggo_word){ jword->u.string.ptr,
            jmeaning->u.string.ptr,
            jexplanation->u.string.ptr};
    }
    munmap(pvoc, statbuf.st_size);
    return LINGGO_OK;
}

linggo_voc_search_results linggo_voc_search(const char* word)
{
    linggo_voc_search_results result;
    result.data = malloc(sizeof(size_t) * 64);
    size_t pos = 0;
    for (int i = 0; i < linggo_voc.voc_size && pos < 64; ++i)
    {
        if (strcmp(word, linggo_voc.lookup_table[i].word) == 0)
            result.data[pos++] = i;
    }
    result.size = pos;
    return result;
}

void linggo_voc_search_free(linggo_voc_search_results result)
{
    free(result.data);
}

void linggo_free_voc()
{
    // todo
}
