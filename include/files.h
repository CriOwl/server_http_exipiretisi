#ifndef FILE_H
#define FILE_H
#define BAD_PATH   -1
#define OK_PATH   0
#include <stddef.h>

struct file_metadata{
    int fd_file;
    size_t len_file;
    long int offset;
    int is_file_not_send;

};
int init_files();
int is_hex(char value);
int decode_path(char *path, size_t len_path, char *path_dst);
int open_fd(char *path, size_t idx_path);
int send_body_http(int fd_socket, struct file_metadata *fil);
int get_len_file(struct file_metadata *fil);
int open_fd_get_len(struct file_metadata *fil,char *path, size_t idx_path);

#endif