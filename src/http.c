/*
Modulo de parseo de una peticion HTTP 
este modulo tomara un arreglo de bytes y separa
los encabezados, comandos, y contenido a la vez que verifica la sintaxis http 
y la disponibilidad de recursos para responder con los codigos de respuesta
correspondiente a la vez que arma los encabezados*/
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
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
};

char get_char(struct http_msg_ *msg, size_t offset){
    return msg->buffer_read[(msg->idx_begin + offset) % BUFFER_READ];
}
int is_char_avaible_header(char value){
         //  A-Z                            ^ - z                           0-9          
    if ((value >= 65 && value <= 90) || (value >= 94 && value <= 122) || (value >= 48 && value <= 57)) {
        return 1;
    }
    // valores ascci de !#$%&'                          *+-                                                         
    else if((value>=33 && value<=39 && value !=34) || (value>=42 && value<=46 && value !=44) || (value==124) || (value==126)){
        return 1;
    }
    return 0;
}

int copy_buffer(char *dst, int size_dst, struct http_msg_ *msg_http, int start, int end){
    if (end < start || end - start >= size_dst) {
        return HTTP_PARSE_TOO_LARGE;
    }
    int n = end - start;
    for (int i = 0; i < n; i++) {
        dst[i] = (char)get_char(msg_http, start + i);
    }
    dst[n] = '\0';
    return HTTP_PARSE_OK;
}

int find_request_header_len_(struct http_msg_ *msg_http, size_t *header_len){
    for (int i = 0; i + 3 < msg_http->len_read; i++) {
        if (get_char(msg_http, i) == '\r' && get_char(msg_http, i + 1) == '\n' && get_char(msg_http, i + 2) == '\r' && get_char(msg_http, i + 3) == '\n') {
            *header_len = i + 4;
            return HTTP_PARSE_OK;
        }
    }
    return HTTP_PARSE_MORE;
}

int find_fin_header(struct http_msg_ *msg_http, int start, int limit, int *idx_fin_header){
    for (int i = start; i < limit; i++) {
        char value = get_char(msg_http, i);
        if (value == '\r') {
            if (i + 1 < limit && get_char(msg_http, i + 1) == '\n') {
                *idx_fin_header = i;
                return HTTP_PARSE_OK;
            }
            return HTTP_PARSE_BAD_REQUEST;
        }
        if (value == '\n') {
            return HTTP_PARSE_BAD_REQUEST;
        }
    }
    return HTTP_PARSE_BAD_REQUEST;
}

void skip_http_request(struct http_msg_ *http_msg, size_t len){
    if (len == 0 || len > http_msg->len_read) {
        return;
    }
    // Recorre todo el buffer circular en caso de fallo o de aceptacion evitando consumir data ya consumida o incorrecta
    http_msg->idx_end = (http_msg->idx_begin + len - 1) % BUFFER_READ; 
    http_msg->idx_begin = (http_msg->idx_begin + len) % BUFFER_READ;
    http_msg->len_read -= len;
}


int parse_first_line( struct http_msg_ *msg_http, size_t line_end){
    size_t i = 0;
    // Un comando de http al inicio no tiene " " solo al final 
    while (i < line_end && get_char(msg_http, i) != ' ') {
        i++;
    }
    if (i == 0 || i == line_end) {
        // la solicitud era solo " " o tenia un " " al inicio que segun el rfc no deberia
        return HTTP_PARSE_BAD_REQUEST;
    }
    for (size_t j = 0; j < i; j++) {
    // El comando http tiene los mismo simboloes q los de headers es [A-z] [0-9] [!#$%^&*-_.+~|*]
        if (!is_char_avaible_header(get_char(msg_http, j))) {
            return HTTP_PARSE_BAD_REQUEST;
        }
    }
    int result = copy_buffer(msg_http->f_line.method,sizeof(msg_http->f_line.method),msg_http, 0, i);
    if (result != HTTP_PARSE_OK) {
        return result;
    }
    msg_http->f_line.idx_method = i;
    if (strcmp(msg_http->f_line.method, "GET") != 0) {
        return HTTP_PARSE_METHOD;
    }
    while (i < line_end && get_char(msg_http, i) == ' ') {
        i++;
    }
    size_t idx_path_begin = i;
    size_t idx_path_end = idx_path_begin;
    while (idx_path_end < line_end && get_char(msg_http, idx_path_end) != ' ') {
        idx_path_end++;
    }
    if (idx_path_begin == idx_path_end || idx_path_end == line_end) {
        return HTTP_PARSE_BAD_REQUEST;
    }
    if (get_char(msg_http, idx_path_begin) != '/') {
        return HTTP_PARSE_BAD_REQUEST;
    }
    for (size_t i = idx_path_begin; i < idx_path_end; i++) {
        char value = get_char(msg_http, i);
        // No se acepta en el frag ni querrys
        if (value <= ' ' || value >= 127 || value == '#') {
            return HTTP_PARSE_BAD_REQUEST;
        }
    }
    result = copy_buffer(msg_http->f_line.path,sizeof(msg_http->f_line.path), msg_http, idx_path_begin, idx_path_end);
    if (result != HTTP_PARSE_OK) {
        return result;
    }
    msg_http->f_line.idx_path = idx_path_end - idx_path_begin;
    i=idx_path_end;
    while (i < line_end && get_char(msg_http, i) == ' ') {
        i++;
    }
    //HTTP/
    if(i+5 >line_end || !(get_char(msg_http, i) == 'H' && get_char(msg_http, i) == 'T' && get_char(msg_http, i) == 'T' && get_char(msg_http, i) == 'P'
        && get_char(msg_http, i) == '/')){
        return HTTP_PARSE_VERSION;
    }
    while (i < line_end && (get_char(msg_http, i) >=48 && get_char(msg_http, i) <=57)) {
        i++;
    }
    if(!get_char(msg_http, i)=='.'){
        return HTTP_PARSE_VERSION;
    }
    i++;
    while (i < line_end && (get_char(msg_http, i) >=48 && get_char(msg_http, i) <=57)) {
        i++;
    }
    return HTTP_PARSE_OK;
}
int is_connection_close(char *value){
    return strstr(value,"close") || strstr(value,"CLOSE");
}


int parse_headers(struct http_msg_ *http_msg, size_t idx, size_t header_len){
    // todo request http necesita el encabezado Host
    int host_found = 0;
    while (idx < header_len) {
        size_t line_end;
        int error = find_fin_header( http_msg, idx, header_len, &line_end);
        if (error != HTTP_PARSE_OK) {
            return error;
        }
        if (line_end == idx) {
            if (host_found) {
                return HTTP_PARSE_OK;
            }
            return HTTP_PARSE_HOST;
        }
        if (http_msg->idx_header >= MAX_HEADER) {
            return HTTP_PARSE_TOO_MANY;
        }
        size_t idx_colon = idx;
        while (idx_colon < line_end && get_char(http_msg, idx_colon) != ':') {
            idx_colon++;
        }
        if (idx_colon == idx || idx_colon == line_end) {
            return HTTP_PARSE_BAD_REQUEST;
        }
        for (int i = idx; i < idx_colon; i++) {
            if (!is_char_avaible_header(get_char(http_msg, i))) {
                return HTTP_PARSE_BAD_REQUEST;
            }
        }
        int value_begin = idx_colon + 1;
        int value_end = line_end;
        while (value_begin < value_end && (get_char(http_msg, value_begin) == ' ' || get_char(http_msg, value_begin) == '\t')) {
            value_begin++;
        }
        while (value_end > value_begin && (get_char(http_msg, value_end - 1) == ' ' || get_char(http_msg, value_end - 1) == '\t')) {
            value_end--;
        }
        for (int i = value_begin; i < value_end; i++) {
            char val = get_char(http_msg, i);
            if (val != '\t' && (val < 32 || val >= 127)) {
                return HTTP_PARSE_BAD_REQUEST;
            }
        }
        struct http_header_ *temp_header = &http_msg->headers_t[http_msg->idx_header];

        error = copy_buffer(temp_header->header, sizeof(temp_header->header),  http_msg,   idx,  idx_colon  );
        if (error != HTTP_PARSE_OK) {
            return error;
        }
        error = copy_buffer( temp_header->value,  sizeof(temp_header->value),  http_msg, value_begin, value_end);
        if (error != HTTP_PARSE_OK) {
            return error;
        }
        temp_header->idx_header = idx_colon - idx;
        temp_header->idx_value = value_end - value_begin;
        http_msg->idx_header++;
        if (strcasecmp(temp_header->header, "Host") == 0) {
            if (host_found || temp_header->idx_value == 0) {
                return HTTP_PARSE_HOST;
            }
            for (size_t i = 0; i < temp_header->idx_value; i++) {
                if (temp_header->value[i] == ' ' ||
                    temp_header->value[i] == '\t') {
                    return HTTP_PARSE_HOST;
                }
            }
            host_found = 1;
        }
        else if (strcasecmp(temp_header->header, "Connection") == 0) {

            if (is_connection_close(temp_header->value)) {
                http_msg->close_after_response = 1;
            }
        }
        idx = line_end + 2;
    }
    return HTTP_PARSE_BAD_REQUEST;
}

void http_init(struct http_msg_ *http_msg)
{
    memset(http_msg, 0, sizeof(*http_msg));
    http_msg->idx_end = -1;
}

int http_write(struct http_msg_ *http_msg, char *data, size_t len_data){
    if (len_data > BUFFER_READ - http_msg->len_read) {
        return HTTP_PARSE_TOO_LARGE;
    }
    if (len_data == 0) {
        return HTTP_PARSE_OK;
    }
    for(size_t i=0;i<len_data;i++){
        // len_read de la anterior
        http_msg->buffer_read[(http_msg->idx_begin+i+http_msg->len_read)%BUFFER_READ]= data[i];
    }
    http_msg->len_read+=len_data;
    return HTTP_PARSE_OK;
}


int http_parse_next(struct http_msg_ *http_msg_){
    size_t header_len = 0;
    int result = find_request_header_len_(http_msg_,&header_len);
    if (result != HTTP_PARSE_OK) {
        http_msg_->idx_end = -1;
        return HTTP_PARSE_MORE;
    }
    memset(&http_msg_->f_line,0,sizeof(http_msg_->f_line));
    http_msg_->idx_header = 0;
    http_msg_->close_after_response = 0;
    size_t first_line_end = 0;
    result = find_fin_header(http_msg_,0,header_len, &first_line_end);
    if (result == HTTP_PARSE_OK) {
        result = parse_first_line(http_msg_,first_line_end);
    }
    if (result == HTTP_PARSE_OK) {
        result = parse_headers(http_msg_,first_line_end + 2,header_len);
    }
    skip_http_request(http_msg_, header_len);
    return result;
}

void main(){

}