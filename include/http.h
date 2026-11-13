#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
#include "include/files.h"
#define BUFFER_READ         40960
#define MAX_HEADER             10
#define MAX_BUFFER_HEADER   4096
#define MAX_BUFFER_VALUE    4096
#define MAX_METHOD            15
#define MAX_PATH             120
#define HTTP_PARSE_OK             1
#define HTTP_PARSE_MORE           0
#define HTTP_PARSE_BAD_REQUEST   -1
#define HTTP_PARSE_METHOD       -25
#define HTTP_PARSE_VERSION      -26
#define HTTP_PARSE_TOO_LARGE    -27
#define HTTP_PARSE_TOO_MANY     -28
#define HTTP_PARSE_HOST         -29

struct http_header_ {
    char header[MAX_BUFFER_HEADER];
    size_t idx_header;
    char value[MAX_BUFFER_VALUE];
    size_t idx_value;
};

struct first_line {
    char method[MAX_METHOD];
    size_t idx_method;
    char path[MAX_PATH];
    size_t idx_path;
};

struct http_msg_ {
    char buffer_read[BUFFER_READ];
    size_t idx_begin;
    int idx_end;
    size_t len_read;
    struct first_line f_line;
    struct http_header_ headers_t[MAX_HEADER];
    size_t idx_header;
    int close_after_response;
    struct file_metadata send_file;   
};
char get_char(struct http_msg_ *msg, size_t offset);
int is_char_avaible_header(char value);
int copy_buffer(char *dst, int size_dst, struct http_msg_ *msg_http, int start, int end);
int find_request_header_len_(struct http_msg_ *msg_http, size_t *header_len);
void skip_http_request(struct http_msg_ *http_msg, size_t len);
int parse_first_line( struct http_msg_ *msg_http, size_t line_end);
int is_connection_close(char *value);
int parse_headers(struct http_msg_ *http_msg, size_t idx, size_t header_len);
void http_init(struct http_msg_ *http_msg);
int http_write(struct http_msg_ *http_msg, char *data, size_t len_data);
int http_parse_next(struct http_msg_ *http_msg);

#endif