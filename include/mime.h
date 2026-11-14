#ifndef MIME_H
#define MIME_H
#include <stddef.h>
int get_mime(char *path,size_t len,char *mime_buffer,size_t len_mime_buffer);
#endif