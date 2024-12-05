#include "linggo/voc.h"
#include "linggo/error.h"

#include "json-parser/json.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

enum LINGGO_CODE linggo_init_voc(const char* voc_path)
{
    int fd = open(voc_path, O_RDONLY);
    struct stat statbuf;
    if (fd < 0 || stat(voc_path, &statbuf) != 0) return LINGGO_INVALID_VOC_PATH;
    char* pvoc = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pvoc == MAP_FAILED) return LINGGO_INVALID_VOC_PATH;
    json_value* json = json_parse(pvoc, statbuf.st_size);

    if (json == NULL || json->type != json_object) return LINGGO_INVALID_CONFIG;

    struct linggo_word_node** curr = &linggo_voc.index;

    for (int i = 0; i < json->u.object.length; ++i)
    {
        *curr = malloc(sizeof(struct linggo_word_node));
        if (*curr == NULL) return LINGGO_OUT_OF_MEMORY;
        memset(*curr,0,sizeof(struct linggo_word_node));
        (*curr)->word = malloc(sizeof(struct linggo_word));
        if ((*curr)->word == NULL) return LINGGO_OUT_OF_MEMORY;
        memset((*curr)->word,0,sizeof(struct linggo_word));

        // init word index

        // todo

        curr = &(*curr)->next;
    }

    munmap(pvoc, statbuf.st_size);
    return LINGGO_OK;
}


enum LINGGO_CODE linggo_free_voc(const char* voc_path)
{
    // todo
    return LINGGO_OK;
}
