/*
Modulo de parseo de una peticion HTTP 
este modulo tomara un arreglo de bytes y separa
los encabezados, comandos, y contenido a la vez que verifica la sintaxis http 
y la disponibilidad de recursos para responder con los codigos de respuesta
correspondiente a la vez que arma los encabezados*/
#define _POSIX_C_SOURCE 200809L
#include "include/mime.h"
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

int get_mime(char *path,size_t len,char *mime_buffer,size_t len_mime_buffer);

int get_mime(char *path,size_t len,char *mime_buffer,size_t len_mime_buffer){
    size_t idx_inicio = 0;
    if(len_mime_buffer<64){
        return -400;
    }
    for (idx_inicio = len - 1; idx_inicio > 0 && path[idx_inicio] != '.'; idx_inicio--);
    char ext[len];
    size_t i = 0;
    if (idx_inicio > 0 && path[idx_inicio] == '.') {
        idx_inicio++;        
        for (i = 0; idx_inicio < len - 1; i++, idx_inicio++) {
            ext[i] = path[idx_inicio];
        }
    }else{
        return -400;
    }
    ext[i] = '\0';
    if (strcmp("html", ext) == 0) {
        snprintf(mime_buffer, len_mime_buffer, "%s", "text/html");
    } else if (strcmp("css", ext) == 0) {
        snprintf(mime_buffer, len_mime_buffer, "%s", "text/css");
    } else if (strcmp("js", ext) == 0) {
        snprintf(mime_buffer, len_mime_buffer, "%s", "application/javascript");
    } else if (strcmp("png", ext) == 0) {
        snprintf(mime_buffer, len_mime_buffer, "%s", "image/png");
    } else if (strcmp("jpeg", ext) == 0) {
        snprintf(mime_buffer, len_mime_buffer, "%s", "image/jpeg");
    } else {
        snprintf(mime_buffer, len_mime_buffer, "%s", "text/plain");
    }
    return 200;
}
