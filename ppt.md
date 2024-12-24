## 项目框架及后端
- Git，错误处理，后端数据处理
```c++
enum LINGGO_CODE linggo_server_init(const char* config_path);
static int on_request(http_conn_t* conn);
```
#### 错误处理
```c++
enum LINGGO_CODE
{
  LINGGO_OK,
  LINGGO_INVALID_CONFIG_PATH,
  LINGGO_INVALID_CONFIG,
  LINGGO_INVALID_VOC_PATH,
  LINGGO_INVALID_VOC,
  LINGGO_INVALID_DB,
  LINGGO_INVALID_DB_PATH,
  LINGGO_OUT_OF_MEMORY,
  LINGGO_USER_NOT_FOUND,
  LINGGO_WRONG_PASSWORD,
  LINGGO_INVALID_USER,
  LINGGO_AIGEN_QUIZ_ERROR,
  LINGGO_UNEXPECTED_ERROR,
};
```
#### strerror
```c++
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
```
#### linggo_server_init
```c++
enum LINGGO_CODE linggo_server_init(const char* config_path)
{
    FILE* fp = fopen(config_path, "r+");
    if (fp == NULL) return LINGGO_INVALID_CONFIG_PATH;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = malloc(size + 1);
    if (buffer == NULL) return LINGGO_OUT_OF_MEMORY;
    fread(buffer, 1, size, fp);
    fclose(fp);
    buffer[size] = '\0';

    json_value* json = json_parse(buffer, size);
    if (json == NULL || json->type != json_object) return LINGGO_INVALID_CONFIG;

    for (int i = 0; i < json->u.object.length; ++i)
    {
        if (strcmp(json->u.object.values[i].name, "resource_path") == 0)
        {
            if (json->u.object.values[i].value->type != json_string)
                return LINGGO_INVALID_CONFIG;
            linggo_svrctx.resource_path = malloc(json->u.object.values[i].value->u.string.length);
            if (linggo_svrctx.resource_path == NULL)
                return LINGGO_OUT_OF_MEMORY;
            strcpy(linggo_svrctx.resource_path, json->u.object.values[i].value->u.string.ptr);
        }
    }

    json_value_free(json);
    free(buffer);

    char dbpath[256];
    strcpy(dbpath, linggo_svrctx.resource_path);
    strcat(dbpath, "/records/linggo.lgdb");

    int ret = linggo_userdb_init(dbpath);
    if (ret == LINGGO_OK)
    {
        printf("Successfully load user database from %s.\n", dbpath);
    }
    else if (ret == LINGGO_INVALID_DB_PATH)
    {
        printf("Warning: User database not found.\n");
    }
    else if (ret == LINGGO_INVALID_DB)
    {
        printf("Warning: Invalid user database.\n");
    }

    char vocpath[256];
    strcpy(vocpath, linggo_svrctx.resource_path);
    strcat(vocpath, "/voc/voc.json");
    assert(linggo_voc_init(vocpath) == LINGGO_OK);

    return LINGGO_OK;
}
```
#### on_request
```c++
    if (strcmp(req->method, "GET") == 0)
    {
        if (linggo_starts_with(req->path, "/api/register"))
        {
            http_request_params params = linggo_parse_params(req->path);
            char* username = linggo_http_find_key(params, "username");
            char* passwd = linggo_http_find_key(params, "passwd");
            if (username == NULL || passwd == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Expected username and password.", req);
            }

            int ret = linggo_user_register(username, passwd);
            if (ret != LINGGO_OK)
                return report_error(conn, "Registration failed.", req);

            linggo_free_params(params);

            // DB WRITE
            linggo_userdb_write();

            return report_ok(conn, req);
        }
        if (linggo_starts_with(req->path, "/api/get_ai_quiz"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.", req);
            }

            char* wordidx = linggo_http_find_key(params, "word_index");
            if (wordidx == NULL)
                return report_error(conn, "Expected word index.", req);

            int idx = atoi(wordidx);

            if (idx >= (int)linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.", req);

            if (idx == -1)
            {
                idx = rand() % linggo_voc.voc_size;
            }

            json_value* quizobj;
            int ret = linggo_user_get_ai_quiz(user, idx, &quizobj);
            if (ret != LINGGO_OK)
            {
                printf("linggo_user_get_ai_quiz(): %s.", linggo_strerror(ret));
                return report_error(conn, "Getting AI quiz failed.", req);
            }

            json_value* resjson = json_object_new(4);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "word", json_string_new(linggo_voc.lookup_table[idx].word));
            json_object_push(resjson, "word_index", json_integer_new(idx));
            json_object_push(resjson, "quiz", quizobj);

            int jsonlen = (int)json_measure(resjson);
            char* buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);

            log_request(req, buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/get_explanation"))
        {
            http_request_params params = linggo_parse_params(req->path);

            char* wordidx = linggo_http_find_key(params, "word_index");
            if (wordidx == NULL)
                return report_error(conn, "Expected word index.", req);

            int idx = atoi(wordidx);

            if (idx < 0 || idx >= (int)linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.", req);

            json_value* resjson = json_object_new(2);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "explanation", json_string_new(linggo_voc.lookup_table[idx].explanation));

            int jsonlen = (int)json_measure(resjson);
            char* buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);

            log_request(req, buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/mark_word"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.", req);
            }

            char* idxstr = linggo_http_find_key(params, "word_index");
            if (idxstr == NULL)
                return report_error(conn, "Expected word index.", req);

            int idx = atoi(idxstr);

            if (idx < 0 || idx >= linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.", req);

            linggo_user_mark_word(user, idx);

            json_value* resjson = json_object_new(1);

            json_object_push(resjson, "status", json_string_new("success"));

            int jsonlen = (int)json_measure(resjson);
            char* buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);

            log_request(req, buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);

            // DB WRITE
            linggo_userdb_write();
            return 200;
        }
        return http_serve_file(conn);
    }
}
```

## 题目生成
```c++
enum LINGGO_CODE linggo_ai_gen_quiz(const char* word, linggo_ai_quiz* quiz);
enum LINGGO_CODE linggo_user_get_quiz(linggo_user* user, uint32_t idx, json_value** quiz);
void linggo_shuffle(void* base, size_t nitems, size_t size);
```
#### `linggo_ai_gen_quiz` in `ai.c`
```c++
void modifyVariableInFile(const char* filename, const char* varName, const char* newValue) {
    FILE *fp_read, *fp_write;
    char buffer[1024];
    char new_line[1024];
    int found = 0;

    // 以只读模式打开Python文件
    fp_read = fopen(filename, "r");
    if (fp_read == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    // 创建临时文件用于写入修改后的内容
    fp_write = fopen("temp.py", "w");
    if (fp_write == NULL)
    {
        fclose(fp_read);
        perror("Error opening temp file for writing");
        exit(1);
    }

    // 逐行读取原文件内容，找到要修改的变量行并修改，其他行原样写入临时文件
    while (fgets(buffer, sizeof(buffer), fp_read))
    {
        if (strstr(buffer, varName) != NULL && found == 0)
        {
            // 构造新的包含变量修改后的行
            sprintf(new_line, "%s = '''%s'''\n", varName, newValue);
            fputs(new_line, fp_write);
            found = 1;
        }
        else
        {
            fputs(buffer, fp_write);
        }
    }

    fclose(fp_read);
    fclose(fp_write);

    if (!found)
    {
        remove("temp.py"); // 如果没找到变量，删除临时文件
        printf("Variable %s not found in %s.\n", varName, filename);
        exit(1);
    }

    // 删除原文件
    remove(filename);
    // 将临时文件重命名为原文件名，实现替换
    rename("temp.py", filename);
}

enum LINGGO_CODE linggo_ai_gen_quiz(const char* word, linggo_ai_quiz* quiz) {
    char question[512] = "\0";
    char filename[64] = "\0";
    
    // 设置文件名和问题 ......

    strcat(question,
           "，以该单词使用json格式输出一道大学英语选择题（仅有题目，abcd四个选项和答案，不包括任何代码和回答注释），json生成示例： { \"question\": \"<Question>\", \"options\": { \"A\": \"<Option A>\", \"B\": \"<Option B>\", \"C\": \"<Option C>\", \"D\": \"<Option D>\" }, \"answer\": \"<Correct Answer>\" }");

    modifyVariableInFile(filename, VARNAME, question);

    char command[512] = "";
    // 设置 python 命令 ...
    system(command);

    char qfilename[64] = "\0";
    char* buffer = malloc ...
    // 设置 python 输出的目录名并读取至 buffer ... 

    json_value* json = json_parse(buffer, size);
    if (json == NULL || json->type != json_object)
        return LINGGO_AIGEN_QUIZ_ERROR;

    json_value* jquestion = linggo_json_find_key(json, "question");
    json_value* janswer = linggo_json_find_key(json, "answer");
    json_value* joptions = linggo_json_find_key(json, "options");
     
    // 设置 json 问题 答案 ......
     
    free(buffer);
    return LINGGO_OK;
}
```
#### `linggo_user_get_quiz` in `user.c`
```c++
enum LINGGO_CODE linggo_user_get_quiz(linggo_user* user, uint32_t idx, json_value** quiz)
{
    if (user == NULL) return LINGGO_INVALID_USER;
    uint32_t word0, word1, word2;

    word0 = rand() % linggo_voc.voc_size;
    word1 = rand() % linggo_voc.voc_size;
    word2 = rand() % linggo_voc.voc_size;

    const char* opt[4] = {"A", "B", "C", "D"};
    linggo_shuffle(opt, 4, sizeof(const char*));

    json_value* json = json_object_new(4);
    json_value* options = json_object_new(4);
    
//  设置 json 中的问题 答案 ...

    *quiz = json;
    return LINGGO_OK;
}
```
#### `linggo_shuffle` in `utils.c`
```c++
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
```
## 词汇查询
```c++
linggo_voc_search_result linggo_voc_search(const char* target)
```
#### `linggo_voc_search` in `voc.c`
```c++
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
    search_node* head = NULL;
    size_t len = 0;

    {
        search_node** curr = &head;

        size_t target_len = strlen(target);
        for (int i = 0; i < linggo_voc.voc_size; ++i)
        {
            if (strcmp(target, linggo_voc.lookup_table[i].word) == 0
                || strstr(target, linggo_voc.lookup_table[i].meaning) != NULL
                || linggo_get_edit_distance(target, linggo_voc.lookup_table[i].word) < target_len / 2
                || linggo_starts_with(linggo_voc.lookup_table[i].word, target))
            {
                *curr = malloc(sizeof(search_node));
                memset(*curr, 0, sizeof(search_node));
                (*curr)->data = i;
                (*curr)->next = NULL;
                curr = &(*curr)->next;
                ++len;
            }
        }
        if (len == 0)
            return (linggo_voc_search_result){NULL, 0};
    }

    // set up result
    linggo_voc_search_result ret;
    ret.data = malloc(sizeof(uint32_t) * len);
    ret.size = len;

    // setup diffs
    diffs_len = len;
    diffs = malloc(sizeof(hash_node) * diffs_len);
    memset(diffs, 0, sizeof(hash_node) * diffs_len);

    int retpos = 0;
    search_node* curr = head;
    while (curr != NULL)
    {
        const char* word = linggo_voc.lookup_table[curr->data].word;
        uint32_t h = hash(word) % diffs_len;

        hash_node** a = &diffs[h];
        while (*a != NULL)
            a = &(*a)->next;
        *a = malloc(sizeof(hash_node));
        (*a)->data = linggo_get_edit_distance(target, word);
        (*a)->raw = word;
        (*a)->next = NULL;

        assert(retpos < diffs_len);
        ret.data[retpos++] = curr->data;

        curr = curr->next;
    }

    qsort(ret.data, len, sizeof(uint32_t), cmp_func);

    // cleanup
    for (uint32_t i = 0; i < len; ++i)
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
```
## 用户管理
```c++
enum LINGGO_CODE linggo_user_register(const char* username, const char* password);
enum LINGGO_CODE linggo_user_login(const char* username, const char* password, linggo_user** user);
enum LINGGO_CODE linggo_user_mark_word(linggo_user* user, uint32_t idx);
enum LINGGO_CODE linggo_user_unmark_word(linggo_user* user, uint32_t idx);
int linggo_user_is_marked_word(linggo_user* user, uint32_t idx);
```
#### `linggo_user_register` in `user.c`
```c++
enum LINGGO_CODE linggo_user_register(const char* username, const char* password)
{
    size_t namelen = strlen(username);
    size_t passwdlen = strlen(password);

    linggo_user** curr = &linggo_userdb.db;

    while (*curr != NULL)
        curr = &(*curr)->next;

    *curr = malloc(sizeof(linggo_user));
    memset(*curr, 0, sizeof(linggo_user));
    (*curr)->uid = linggo_userdb.next_uid++;
    (*curr)->name = malloc(namelen + 1);
    memcpy((*curr)->name, username, namelen);
    (*curr)->name[namelen] = '\0';
    (*curr)->passwd = malloc(passwdlen + 1);
    memcpy((*curr)->passwd, password, passwdlen);
    (*curr)->passwd[passwdlen] = '\0';
    (*curr)->next = NULL;
    return LINGGO_OK;
}
```
#### `linggo_user_login` in `user.c`
```c++
enum LINGGO_CODE linggo_user_login(const char* username, const char* password, linggo_user** user)
{
    linggo_user* curr = linggo_userdb.db;
    while (curr != NULL)
    {
        if (strcmp(curr->name, username) == 0)
        {
            if (strcmp(curr->passwd, password) == 0)
            {
                if (user != NULL)
                    *user = curr;
                return LINGGO_OK;
            }
            else
                return LINGGO_WRONG_PASSWORD;
        }
        curr = curr->next;
    }
    return LINGGO_USER_NOT_FOUND;
}
```
#### `linggo_user_mark_word` in `user.c`
```c++
enum LINGGO_CODE linggo_user_mark_word(linggo_user* user, uint32_t idx)
{
    if (user == NULL) return LINGGO_INVALID_USER;
    linggo_marked_word** curr = &user->marked_words;

    while (*curr != NULL)
    {
        if ((*curr)->idx == idx)
            return LINGGO_OK;
        curr = &(*curr)->next;
    }

    *curr = malloc(sizeof(linggo_marked_word));
    memset(*curr, 0, sizeof(linggo_marked_word));
    (*curr)->idx = idx;
    (*curr)->next = NULL;
    return LINGGO_OK;
}
```

#### `linggo_user_unmark_word` in `user.c`
```c++
enum LINGGO_CODE linggo_user_unmark_word(linggo_user* user, uint32_t idx)
{
    if (user == NULL) return LINGGO_INVALID_USER;
    linggo_marked_word* curr = user->marked_words;
    linggo_marked_word* prev = NULL;

    while (curr != NULL)
    {
        if (curr->idx == idx)
        {
            if (prev != NULL)
                prev->next = curr->next;
            else
                user->marked_words = NULL;

            free(curr);
            return LINGGO_OK;
        }
        prev = curr;
        curr = curr->next;
    }
    return LINGGO_OK;
}
```
#### `linggo_user_is_marked_word` in `user.c`
```c++
int linggo_user_is_marked_word(linggo_user* user, uint32_t idx)
{
    if (user == NULL) return -1;
    const linggo_marked_word* curr = user->marked_words;
    while (curr != NULL)
    {
        if (curr->idx == idx)
            return 1;
        curr = curr->next;
    }
    return 0;
}
```
## 数据管理
```c++ 
enum LINGGO_CODE linggo_userdb_init(const char* db_path);
enum LINGGO_CODE linggo_userdb_write();
```
#### `linggo_userdb_init` in `user.c`
```c++
    char db_magic[5];
    size_t nread = fread(db_magic, sizeof(char), sizeof(db_magic), file);

    if (nread != sizeof(db_magic) ||
        db_magic[0] != 0x7f || db_magic[1] != 'L'
        || db_magic[2] != 'G' || db_magic[3] != 'D'
        || db_magic[4] != 'B')
    {
//        错误处理
    }

    nread = fread(&linggo_userdb.next_uid, sizeof(linggo_userdb.next_uid), 1, file);
    if (nread != 1)
    {
//        错误处理
    }

    linggo_user** curr_user = &linggo_userdb.db;
    while (1)
    {
        size_t curr_user_len;
        nread = fread(&curr_user_len, sizeof(curr_user_len), 1, file);
        if (nread != 1)
//          错误处理

        char* buffer = malloc(curr_user_len);

        nread = fread(buffer, sizeof(char), curr_user_len, file);
        if (nread != curr_user_len)
//          错误处理

        char* ptr = buffer;
        uint32_t uid = *(uint32_t*)ptr;
        ptr += sizeof(uid);
        (*curr_user)->uid = uid;
        
//      用户名，密码 .......

        if (ptr - buffer > curr_user_len)
//          错误处理

        linggo_marked_word** curr_word = &(*curr_user)->marked_words;

        while (ptr - buffer < curr_user_len)
        {
            *curr_word = malloc(sizeof(linggo_marked_word));
            memset(*curr_word, 0, sizeof(linggo_marked_word));
            (*curr_word)->idx = *(uint32_t*)ptr;
            (*curr_word)->next = NULL;
            curr_word = &(*curr_word)->next;
            ptr += sizeof(uint32_t);
        }

        if (ptr - buffer != curr_user_len)
//          错误处理
        curr_user = &(*curr_user)->next;
//          内存回收        
        free(buffer);
    }
    return LINGGO_OK;
```
#### `linggo_userdb_write` in `user.c`
```c++
    // DB Header
    char db_magic[5] = {0x7f, 'L', 'G', 'D', 'B'};
    fwrite(db_magic, sizeof(db_magic), sizeof(char), file);
    fwrite(&linggo_userdb.next_uid, sizeof(linggo_userdb.next_uid), 1, file);

    // 用户数据
    linggo_user* curr_user = linggo_userdb.db;
    while (curr_user != NULL)
    {
        long length_pos = ftell(file);
        size_t len = 0;
        fwrite(&len, sizeof(len), 1, file);

        fwrite(&curr_user->uid, sizeof(curr_user->uid), 1, file);
        // 其他用户数据
        linggo_marked_word* curr_word = curr_user->marked_words;
        while (curr_word != NULL)
        {
            fwrite(&curr_word->idx, sizeof(curr_word->idx), 1, file);
            curr_word = curr_word->next;
        }

        long now = ftell(file);
        len = now - length_pos - sizeof(len);
        fseek(file, length_pos, SEEK_SET);
        fwrite(&len, sizeof(len), 1, file);
        fseek(file, now, SEEK_SET);

        curr_user = curr_user->next;
    }
    fclose(file);
    return LINGGO_OK;
}
```