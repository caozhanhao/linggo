// This file is based on [libhv's example](https://github.com/ithewei/libhv/blob/master/examples/tinyhttpd.c).
// Edited for Linggo Server
#include "linggo/server.h"
#include "linggo/error.h"
#include "linggo/utils.h"

#include "json-parser/json.h"
#include "json-builder/json-builder.h"

#include <hv/hv.h>
#include <hv/hloop.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linggo/voc.h>

#include "linggo/user.h"

static int thread_num = 1;
static hloop_t*  accept_loop = NULL;
static hloop_t** worker_loops = NULL;

#define HTTP_KEEPALIVE_TIMEOUT  60000 // ms
#define HTTP_MAX_URL_LENGTH     256
#define HTTP_MAX_HEAD_LENGTH    1024

#define HTML_TAG_BEGIN  "<html><body><center><h1>"
#define HTML_TAG_END    "</h1></center></body></html>"

// status_message
#define HTTP_OK         "OK"
#define NOT_FOUND       "Not Found"
#define NOT_IMPLEMENTED "Not Implemented"

// Content-Type
#define TEXT_PLAIN       "text/plain"
#define TEXT_HTML        "text/html"
#define TEXT_JAVASCRIPT  "text/javascript"
#define APPLICATION_JSON "application/json"
#define TEXT_CSS         "text/css"

#define PUBLIC_RELATIVE_PATH "/public/"

#define LINGGO_CFG_GET_STR(path, dest)                                                      \
if (strcmp(json->u.object.values[i].name, path) == 0) {                                     \
    if (json->u.object.values[i].value->type != json_string) return LINGGO_INVALID_CONFIG;  \
    dest = malloc(json->u.object.values[i].value->u.string.length);                         \
    if (dest == NULL) return LINGGO_OUT_OF_MEMORY;                                          \
    strcpy(dest, json->u.object.values[i].value->u.string.ptr);                             \
}                                                                                           \

#define LINGGO_CFG_GET_INT(path, dest)                                                       \
if (strcmp(json->u.object.values[i].name, path) == 0) {                                      \
    if (json->u.object.values[i].value->type != json_integer) return LINGGO_INVALID_CONFIG;  \
    dest = (int)json->u.object.values[i].value->u.integer;                                   \
}                                                                                            \

linggo_server_context linggo_svrctx;

enum LINGGO_CODE linggo_server_init(const char* config_path)
{
    FILE* fp = fopen(config_path, "r");
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
        LINGGO_CFG_GET_STR("resource_path", linggo_svrctx.resource_path)
        else LINGGO_CFG_GET_STR("admin_password", linggo_svrctx.admin_password)
        else LINGGO_CFG_GET_STR("listen_address", linggo_svrctx.listen_address)
        else LINGGO_CFG_GET_INT("listen_port", linggo_svrctx.listen_port)
    }

    json_value_free(json);
    free(buffer);

    assert(linggo_userdb_init() == LINGGO_OK);

    char vocpath[256];
    strcpy(vocpath, linggo_svrctx.resource_path);
    strcat(vocpath, "/voc/voc.json");
    assert(linggo_voc_init(vocpath) == LINGGO_OK);

    return 0;
}

typedef enum {
    s_begin,
    s_first_line,
    s_request_line = s_first_line,
    s_status_line = s_first_line,
    s_head,
    s_head_end,
    s_body,
    s_end
} http_state_e;

typedef struct {
    // first line
    int             major_version;
    int             minor_version;
    union {
        // request line
        struct {
            char method[32];
            char path[HTTP_MAX_URL_LENGTH];
        };
        // status line
        struct {
            int  status_code;
            char status_message[64];
        };
    };
    // headers
    char        host[64];
    int         content_length;
    char        content_type[64];
    unsigned    keepalive:  1;
//  char        head[HTTP_MAX_HEAD_LENGTH];
//  int         head_len;
    // body
    char*       body;
    int         body_len; // body_len = content_length
} http_msg_t;

typedef struct {
    hio_t*          io;
    http_state_e    state;
    http_msg_t      request;
    http_msg_t      response;
    // for http_serve_file
    FILE*           fp;
    hbuf_t          filebuf;
} http_conn_t;

static char s_date[32] = {0};
static void update_date(htimer_t* timer) {
    uint64_t now = hloop_now(hevent_loop(timer));
    gmtime_fmt(now, s_date);
}

static int http_response_dump(http_msg_t* msg, char* buf, int len) {
    int offset = 0;
    // status line
    offset += snprintf(buf + offset, len - offset, "HTTP/%d.%d %d %s\r\n", msg->major_version, msg->minor_version, msg->status_code, msg->status_message);
    // headers
    offset += snprintf(buf + offset, len - offset, "Server: libhv/%s\r\n", hv_version());
    offset += snprintf(buf + offset, len - offset, "Connection: %s\r\n", msg->keepalive ? "keep-alive" : "close");
    if (msg->content_length > 0) {
        offset += snprintf(buf + offset, len - offset, "Content-Length: %d\r\n", msg->content_length);
    }
    if (*msg->content_type) {
        offset += snprintf(buf + offset, len - offset, "Content-Type: %s\r\n", msg->content_type);
    }
    if (*s_date) {
        offset += snprintf(buf + offset, len - offset, "Date: %s\r\n", s_date);
    }

    // RESPONSE HEADER

    offset += snprintf(buf + offset, len - offset, "\r\n");
    // body
    if (msg->body && msg->content_length > 0) {
        memcpy(buf + offset, msg->body, msg->content_length);
        offset += msg->content_length;
    }
    return offset;
}

static int http_reply(http_conn_t* conn,
            int status_code, const char* status_message,
            const char* content_type,
            const char* body, int body_len) {
    http_msg_t* req  = &conn->request;
    http_msg_t* resp = &conn->response;
    resp->major_version = req->major_version;
    resp->minor_version = req->minor_version;
    resp->status_code = status_code;
    if (status_message) strncpy(resp->status_message, status_message, sizeof(req->status_message) - 1);
    if (content_type)   strncpy(resp->content_type, content_type, sizeof(req->content_type) - 1);
    resp->keepalive = req->keepalive;
    if (body) {
        if (body_len <= 0) body_len = strlen(body);
        resp->content_length = body_len;
        resp->body = (char*)body;
    }
    char* buf = NULL;
    STACK_OR_HEAP_ALLOC(buf, HTTP_MAX_HEAD_LENGTH + resp->content_length, HTTP_MAX_HEAD_LENGTH + 1024);
    int msglen = http_response_dump(resp, buf, HTTP_MAX_HEAD_LENGTH + resp->content_length);
    int nwrite = hio_write(conn->io, buf, msglen);
    STACK_OR_HEAP_FREE(buf);
    return nwrite < 0 ? nwrite : msglen;
}

static void http_send_file(http_conn_t* conn) {
    if (!conn || !conn->fp) return;
    // alloc filebuf
    if (!conn->filebuf.base) {
        conn->filebuf.len = 4096;
        HV_ALLOC(conn->filebuf, conn->filebuf.len);
    }
    char* filebuf = conn->filebuf.base;
    size_t filebuflen = conn->filebuf.len;
    // read file
    int nread = fread(filebuf, 1, filebuflen, conn->fp);
    if (nread <= 0) {
        // eof or error
        hio_close(conn->io);
        return;
    }
    // send file
    hio_write(conn->io, filebuf, nread);
}

static void on_write(hio_t* io, const void* buf, int writebytes) {
    if (!io) return;
    if (!hio_write_is_complete(io)) return;
    http_conn_t* conn = (http_conn_t*)hevent_userdata(io);
    http_send_file(conn);
}

static int http_serve_file(http_conn_t* conn) {
    http_msg_t* req = &conn->request;
    http_msg_t* resp = &conn->response;
    // GET / HTTP/1.1\r\n
    const char* filepath = req->path + 1;
    // homepage
    if (*filepath == '\0') {
        filepath = "index.html";
    }

    int length = strlen(linggo_svrctx.resource_path) + strlen(filepath) + sizeof(PUBLIC_RELATIVE_PATH);

    char* pathbuf = malloc(length);

    strcpy(pathbuf, linggo_svrctx.resource_path);
    strcat(pathbuf, PUBLIC_RELATIVE_PATH);
    strcat(pathbuf, filepath);

    // open file
    conn->fp = fopen(pathbuf, "rb");
    free(pathbuf);
    if (!conn->fp) {
        http_reply(conn, 404, NOT_FOUND, TEXT_HTML, HTML_TAG_BEGIN NOT_FOUND HTML_TAG_END, 0);
        return 404;
    }
    // send head
    size_t filesize = hv_filesize(filepath);
    resp->content_length = filesize;
    const char* suffix = hv_suffixname(filepath);
    const char* content_type = NULL;

    if (strcmp(suffix, "html") == 0) {
        content_type = TEXT_HTML;
    }
    else if (strcmp(suffix, "css") == 0) {
        content_type = TEXT_CSS;
    }
    else if (strcmp(suffix, "js") == 0) {
        content_type = TEXT_JAVASCRIPT;
    }

    hio_setcb_write(conn->io, on_write);
    int nwrite = http_reply(conn, 200, "OK", content_type, NULL, 0);
    if (nwrite < 0) return nwrite; // disconnected
    return 200;
}

static bool parse_http_request_line(http_conn_t* conn, char* buf, int len) {
    // GET / HTTP/1.1
    http_msg_t* req = &conn->request;
    sscanf(buf, "%s %s HTTP/%d.%d", req->method, req->path, &req->major_version, &req->minor_version);
    if (req->major_version != 1) return false;
    if (req->minor_version == 1) req->keepalive = 1;
    // printf("%s %s HTTP/%d.%d\r\n", req->method, req->path, req->major_version, req->minor_version);
    return true;
}

static bool parse_http_head(http_conn_t* conn, char* buf, int len) {
    http_msg_t* req = &conn->request;
    const char* key = buf;
    const char* val = buf;
    char* delim = strchr(buf, ':');
    if (!delim) return false;
    *delim = '\0';
    val = delim + 1;
    // trim space
    while (*val == ' ') ++val;

    // HEADER
    if (stricmp(key, "Content-Length") == 0) {
        req->content_length = atoi(val);
    } else if (stricmp(key, "Content-Type") == 0) {
        strncpy(req->content_type, val, sizeof(req->content_type) - 1);
    } else if (stricmp(key, "Connection") == 0) {
        if (stricmp(val, "close") == 0) {
            req->keepalive = 0;
        }
    }

    return true;
}

static int report_error(http_conn_t* conn, char* msg) {
    json_value * resjson = json_object_new(2);
    json_value * status = json_string_new("failed");
    json_value * message = json_string_new(msg);
    json_object_push(resjson, "status", status);
    json_object_push(resjson, "message", message);
    int jsonlen = (int)json_measure(resjson);
    char * buf = malloc(jsonlen);
    json_serialize(buf, resjson);
    http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
    printf("Response Sent\nbody: %s\n--------------------\n", buf);
    json_builder_free(resjson);
    free(buf);
    return 200;
}

static int report_ok(http_conn_t* conn) {
    json_value * resjson = json_object_new(1);
    json_value * status = json_string_new("success");
    json_object_push(resjson, "status", status);
    int jsonlen = (int)json_measure(resjson);
    char * buf = malloc(jsonlen);
    json_serialize(buf, resjson);
    http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
    printf("Response Sent\nbody: %s\n--------------------\n", buf);
    json_builder_free(resjson);
    free(buf);
    return 200;
}

static linggo_user* authorize(http_request_params params) {
    char* username = linggo_http_find_key(params, "username");
    char* passwd = linggo_http_find_key(params, "passwd");
    if (username == NULL || passwd == NULL)
        return NULL;
    linggo_user* user;
    int ret = linggo_user_login(username, passwd, &user);
    if (ret != LINGGO_OK)
        return NULL;
    return user;
}

static int on_request(http_conn_t* conn) {
    http_msg_t* req = &conn->request;
    printf("Request Received\nmethod: %s\npath: %s\nbody: %s\n--------------------\n", req->method, req->path, req->body);
    if (strcmp(req->method, "GET") == 0)
    {
        if (linggo_starts_with(req->path, "/api/register")) {
            http_request_params params = linggo_parse_params(req->path);
            char* username = linggo_http_find_key(params, "username");
            char* passwd = linggo_http_find_key(params, "passwd");
            if (username == NULL || passwd == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Expected username and password.");
            }

            int ret = linggo_user_register(username, passwd);
            if (ret != LINGGO_OK)
                return report_error(conn, "Registration failed.");

            linggo_free_params(params);
            return report_ok(conn);
        }
        if (linggo_starts_with(req->path, "/api/login"))
        {
            http_request_params params = linggo_parse_params(req->path);
            char* username = linggo_http_find_key(params, "username");
            char* passwd = linggo_http_find_key(params, "passwd");
            if (username == NULL || passwd == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Expected username and password.");
            }
            int ret = linggo_user_login(username, passwd, NULL);
            linggo_free_params(params);
            if (ret == LINGGO_OK)
                return report_ok(conn);
            else if (ret == LINGGO_WRONG_PASSWORD)
                return report_error(conn, "Incorrect password.");
            else if (ret == LINGGO_USER_NOT_FOUND)
                return report_error(conn, "User not found.");
            else
                return report_error(conn, "Login failed.");
        }
        if (linggo_starts_with(req->path, "/api/get_quiz"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            char* wordidx = linggo_http_find_key(params, "word_index");
            if (wordidx == NULL)
                return report_error(conn, "Expected word index.");

            int idx = atoi(wordidx);

            if (idx >= (int)linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.");

            if (idx == -1)
            {
                idx = rand() % linggo_voc.voc_size;
            }

            json_value* quizobj;
            int ret = linggo_user_get_quiz(user, idx, &quizobj);
            if (ret != LINGGO_OK)
                return report_error(conn, "Getting quiz failed.");

            json_value * resjson = json_object_new(4);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "word", json_string_new(linggo_voc.lookup_table[idx].word));
            json_object_push(resjson, "word_index", json_integer_new(idx));
            json_object_push(resjson, "quiz", quizobj);

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

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
                return report_error(conn, "Expected word index.");

            int idx = atoi(wordidx);

            if (idx < 0 || idx >= (int)linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.");

            json_value * resjson = json_object_new(2);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "explanation", json_string_new(linggo_voc.lookup_table[idx].explanation));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/quiz_prompt"))
        {
            http_request_params params = linggo_parse_params(req->path);

            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            char* astr = linggo_http_find_key(params, "A_index");
            char* bstr = linggo_http_find_key(params, "B_index");
            char* cstr = linggo_http_find_key(params, "C_index");
            char* dstr = linggo_http_find_key(params, "D_index");
            if (astr == NULL || bstr == NULL || cstr == NULL || dstr == NULL)
                return report_error(conn, "Expected word index.");

            int a = atoi(astr);
            int b = atoi(bstr);
            int c = atoi(cstr);
            int d = atoi(dstr);

            if ((a < 0 || a >= (int)linggo_voc.voc_size)
                || (b < 0 || b >= (int)linggo_voc.voc_size)
                || (c < 0 || c >= (int)linggo_voc.voc_size)
                || (d < 0 || d >= (int)linggo_voc.voc_size))
                return report_error(conn, "Word index out of range.");

            json_value * resjson = json_object_new(5);
            json_object_push(resjson, "status", json_string_new("success"));

            json_value * aobj = json_object_new(2);
            json_value * bobj = json_object_new(2);
            json_value * cobj = json_object_new(2);
            json_value * dobj = json_object_new(2);

            json_object_push(aobj, "is_marked", json_boolean_new(linggo_user_is_marked_word(user, a)));
            json_object_push(aobj, "explanation", json_string_new(linggo_voc.lookup_table[a].explanation));

            json_object_push(bobj, "is_marked", json_boolean_new(linggo_user_is_marked_word(user, b)));
            json_object_push(bobj, "explanation", json_string_new(linggo_voc.lookup_table[b].explanation));

            json_object_push(cobj, "is_marked", json_boolean_new(linggo_user_is_marked_word(user, c)));
            json_object_push(cobj, "explanation", json_string_new(linggo_voc.lookup_table[c].explanation));

            json_object_push(dobj, "is_marked", json_boolean_new(linggo_user_is_marked_word(user, d)));
            json_object_push(dobj, "explanation", json_string_new(linggo_voc.lookup_table[d].explanation));

            json_object_push(resjson, "A", aobj);
            json_object_push(resjson, "B", bobj);
            json_object_push(resjson, "C", cobj);
            json_object_push(resjson, "D", dobj);

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/curr_memorize_word"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            linggo_word* word = &linggo_voc.lookup_table[user->curr_memorize_word];

            json_value * resjson = json_object_new(5);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "word", json_string_new(word->word));
            json_object_push(resjson, "meaning", json_string_new(word->meaning));
            json_object_push(resjson, "word_index", json_integer_new(user->curr_memorize_word));
            json_object_push(resjson, "content", json_string_new(word->explanation));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/prev_memorize_word"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            if (user->curr_memorize_word > 0)
                --user->curr_memorize_word;
            else
                user->curr_memorize_word = linggo_voc.voc_size - 1;

            linggo_word* word = &linggo_voc.lookup_table[user->curr_memorize_word];

            json_value * resjson = json_object_new(5);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "word", json_string_new(word->word));
            json_object_push(resjson, "meaning", json_string_new(word->meaning));
            json_object_push(resjson, "word_index", json_integer_new(user->curr_memorize_word));
            json_object_push(resjson, "content", json_string_new(word->explanation));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/next_memorize_word"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            if (user->curr_memorize_word < linggo_voc.voc_size - 1)
                ++user->curr_memorize_word;
            else
                user->curr_memorize_word = 0;

            linggo_word* word = &linggo_voc.lookup_table[user->curr_memorize_word];

            json_value * resjson = json_object_new(5);

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "word", json_string_new(word->word));
            json_object_push(resjson, "meaning", json_string_new(word->meaning));
            json_object_push(resjson, "word_index", json_integer_new(user->curr_memorize_word));
            json_object_push(resjson, "content", json_string_new(word->explanation));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

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
                return report_error(conn, "Unauthorized.");
            }

            char* idxstr = linggo_http_find_key(params, "word_index");
            if (idxstr == NULL)
                return report_error(conn, "Expected word index.");

            int idx = atoi(idxstr);

            if (idx < 0 || idx >= linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.");

            linggo_user_mark_word(user, idx);

            json_value * resjson = json_object_new(1);

            json_object_push(resjson, "status", json_string_new("success"));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/unmark_word"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            char* idxstr = linggo_http_find_key(params, "word_index");
            if (idxstr == NULL)
                return report_error(conn, "Expected word index.");

            int idx = atoi(idxstr);

            if (idx < 0 || idx >= linggo_voc.voc_size)
                return report_error(conn, "Word index out of range.");

            linggo_user_unmark_word(user, idx);

            json_value * resjson = json_object_new(1);

            json_object_push(resjson, "status", json_string_new("success"));

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/get_marked"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            json_value * resjson = json_object_new(2);

            int len = 0;
            linggo_marked_word* word = user->marked_words;
            while (word != NULL)
            {
                ++len;
                word = word->next;
            }

            json_value* marked_words = json_array_new(len);

            word = user->marked_words;
            while (word != NULL)
            {
                json_value* mword = json_object_new(3);
                json_object_push(mword, "word", json_string_new(linggo_voc.lookup_table[word->idx].word));
                json_object_push(mword, "meaning", json_string_new(linggo_voc.lookup_table[word->idx].meaning));
                json_object_push(mword, "word_index", json_integer_new(word->idx));
                json_array_push(marked_words, mword);
                word = word->next;
            }

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "marked_words", marked_words);

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        if (linggo_starts_with(req->path, "/api/search"))
        {
            http_request_params params = linggo_parse_params(req->path);
            linggo_user* user = authorize(params);
            if (user == NULL)
            {
                linggo_free_params(params);
                return report_error(conn, "Unauthorized.");
            }

            char* word = linggo_http_find_key(params, "word");

            if (word == NULL)
                return report_error(conn, "Expected word to search.");

            json_value * resjson = json_object_new(2);

            linggo_voc_search_results search_results = linggo_voc_search(word);
            json_value * words = json_array_new(search_results.size);
            for (int i = 0; i < search_results.size; ++i)
            {
                int idx = search_results.data[i];
                json_value* mword = json_object_new(3);
                json_object_push(mword, "word", json_string_new(linggo_voc.lookup_table[idx].word));
                json_object_push(mword, "meaning", json_string_new(linggo_voc.lookup_table[idx].meaning));
                json_object_push(mword, "word_index", json_integer_new(idx));
                json_array_push(words, mword);
            }

            json_object_push(resjson, "status", json_string_new("success"));
            json_object_push(resjson, "words", words);

            int jsonlen = (int)json_measure(resjson);
            char * buf = malloc(jsonlen);
            json_serialize(buf, resjson);
            http_reply(conn, 200, HTTP_OK, APPLICATION_JSON, buf, jsonlen - 1);
            printf("Response Sent\nbody: %s\n--------------------\n", buf);

            linggo_voc_search_free(search_results);
            linggo_free_params(params);
            json_builder_free(resjson);
            free(buf);
            return 200;
        }
        return http_serve_file(conn);
    }
    if (strcmp(req->method, "POST") == 0) {
        http_reply(conn, 501, NOT_IMPLEMENTED, TEXT_HTML, HTML_TAG_BEGIN NOT_IMPLEMENTED HTML_TAG_END, 0);
    }
    http_reply(conn, 501, NOT_IMPLEMENTED, TEXT_HTML, HTML_TAG_BEGIN NOT_IMPLEMENTED HTML_TAG_END, 0);
    return 501;
}

static void on_close(hio_t* io) {
    http_conn_t* conn = (http_conn_t*)hevent_userdata(io);
    if (conn) {
        if (conn->fp) {
            // close file
            fclose(conn->fp);
            conn->fp = NULL;
        }
        // free filebuf
        HV_FREE(conn->filebuf.base);
        HV_FREE(conn);
        hevent_set_userdata(io, NULL);
    }
}

static void on_recv(hio_t* io, void* buf, int readbytes) {
    char* str = buf;
    http_conn_t* conn = (http_conn_t*)hevent_userdata(io);
    http_msg_t* req = &conn->request;
    switch (conn->state) {
    case s_begin:
        conn->state = s_first_line;
    case s_first_line:
        if (readbytes < 2) {
            fprintf(stderr, "Not match \r\n!");
            hio_close(io);
            return;
        }
        str[readbytes - 2] = '\0';
        if (parse_http_request_line(conn, str, readbytes - 2) == false) {
            fprintf(stderr, "Failed to parse http request line:\n%s\n", str);
            hio_close(io);
            return;
        }
        // start read head
        conn->state = s_head;
        hio_readline(io);
        break;
    case s_head:
        if (readbytes < 2) {
            fprintf(stderr, "Not match \r\n!");
            hio_close(io);
            return;
        }
        if (readbytes == 2 && str[0] == '\r' && str[1] == '\n') {
            conn->state = s_head_end;
        } else {
            str[readbytes - 2] = '\0';
            if (parse_http_head(conn, str, readbytes - 2) == false) {
                fprintf(stderr, "Failed to parse http head:\n%s\n", str);
                hio_close(io);
                return;
            }
            hio_readline(io);
            break;
        }
    case s_head_end:
        if (req->content_length == 0) {
            conn->state = s_end;
            goto s_end;
        } else {
            // start read body
            conn->state = s_body;
            // WARN: too large content_length should read multiple times!
            hio_readbytes(io, req->content_length);
            break;
        }
    case s_body:
        req->body = str;
        req->body_len += readbytes;
        if (req->body_len == req->content_length) {
            conn->state = s_end;
        } else {
            // WARN: too large content_length should be handled by streaming!
            break;
        }
    case s_end:
s_end:
        // received complete request
        on_request(conn);
        if (hio_is_closed(io)) return;
        if (req->keepalive) {
            // Connection: keep-alive\r\n
            // reset and receive next request
            memset(&conn->request,  0, sizeof(http_msg_t));
            memset(&conn->response, 0, sizeof(http_msg_t));
            conn->state = s_first_line;
            hio_readline(io);
        } else {
            // Connection: close\r\n
            hio_close(io);
        }
        break;
    default: break;
    }
}

static void new_conn_event(hevent_t* ev) {
    hloop_t* loop = ev->loop;
    hio_t* io = (hio_t*)hevent_userdata(ev);
    hio_attach(loop, io);

    hio_setcb_close(io, on_close);
    hio_setcb_read(io, on_recv);
    hio_set_keepalive_timeout(io, HTTP_KEEPALIVE_TIMEOUT);

    http_conn_t* conn = NULL;
    HV_ALLOC_SIZEOF(conn);
    conn->io = io;
    hevent_set_userdata(io, conn);
    // start read first line
    conn->state = s_first_line;
    hio_readline(io);
}

static hloop_t* get_next_loop() {
    static int s_cur_index = 0;
    if (s_cur_index == thread_num) {
        s_cur_index = 0;
    }
    return worker_loops[s_cur_index++];
}

static void on_accept(hio_t* io) {
    hio_detach(io);

    hloop_t* worker_loop = get_next_loop();
    hevent_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.loop = worker_loop;
    ev.cb = new_conn_event;
    ev.userdata = io;
    hloop_post_event(worker_loop, &ev);
}

static HTHREAD_ROUTINE(worker_thread) {
    hloop_t* loop = (hloop_t*)userdata;
    hloop_run(loop);
    return 0;
}

static HTHREAD_ROUTINE(accept_thread) {
    hloop_t* loop = (hloop_t*)userdata;
    hio_t* listenio = hloop_create_tcp_server(loop, linggo_svrctx.listen_address, linggo_svrctx.listen_port, on_accept);
    if (listenio == NULL) {
        exit(1);
    }
    printf("LingGo Server started successfully. (listening on %s:%d, listenfd=%d, thread_num=%d)\n",
            linggo_svrctx.listen_address, linggo_svrctx.listen_port, hio_fd(listenio), thread_num);
    // NOTE: add timer to update date every 1s
    htimer_add(loop, update_date, 1000, INFINITE);
    hloop_run(loop);
    return 0;
}

enum LINGGO_CODE linggo_server_start()
{
    if (thread_num == 0) thread_num = 1;

    worker_loops = (hloop_t**)malloc(sizeof(hloop_t*) * thread_num);
    for (int i = 0; i < thread_num; ++i) {
        worker_loops[i] = hloop_new(HLOOP_FLAG_AUTO_FREE);
        hthread_create(worker_thread, worker_loops[i]);
    }

    accept_loop = hloop_new(HLOOP_FLAG_AUTO_FREE);
    accept_thread(accept_loop);
    return LINGGO_OK;
}