#include "include/files.h"
#include <fcntl.h>         
#include <linux/openat2.h>
#include <stdio.h>
#include <sys/syscall.h>  
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/sendfile.h>    
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#define BAD_PATH   -1
#define OK_PATH   0

struct file_metadata{
    size_t len_file;
    int fd;
    long int offset;
};

int init_files();
int is_hex(char value);
int decode_path(char *path, size_t len_path, char *path_dst);
int open_fd(char *path, size_t idx_path);
int send_body_http(int fd_socket, struct file_metadata *fil);
int get_len_file(struct file_metadata *fil);

int fd_dir=-1;
int is_mode_server=0;
struct open_how rules={0};
const char *web_cd="./www/";

int init_files(){
    fd_dir=open(web_cd,__O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if(fd_dir<=0){
        perror("No se puede crear el fd del directorio: ");
    }
    rules.flags=O_RDONLY | O_CLOEXEC;
    rules.resolve=RESOLVE_NO_SYMLINKS | RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS;
    if(is_mode_server){
        close(0);
        close(1);
        close(2);
    }
}

int is_hex(char value){
    return (value>='0' && value<='9') || (value>='a' && value<='f') || (value>='A' && value<='F');
}


int decode_path(char *path, size_t len_path, char *path_dst){
    int idx=0;
    for(size_t i=0; i<len_path;i++,idx++){
        if(path[i]=='%'){
            if(i+2<len_path && is_hex(path[i+1]) && is_hex(path[i+2])){
                char temp[3]={path[i+1],path[i+2],'\0'};
                int value=strtol(temp,NULL,16);
                char letter=(char) value;
                if(letter>=32 && letter<=126){
                    path_dst[idx]=letter;
                    i=i+2;
                    continue;
                }
            }
            return BAD_PATH;
        }
        path_dst[idx]=path[i];
    }
    
    return OK_PATH;
}

int open_fd(char *path, size_t idx_path){
    char path_decode[idx_path+1];
    int error=decode_path(path,idx_path+1,path_decode);
    if(error!=0){
        return BAD_PATH;
    }
    int fd=syscall(SYS_openat2,fd_dir,path_decode,&rules,sizeof(rules));
    if(fd<0){
        switch (errno){
        case ENOENT: // Ruta no existe
            return -404;
        case ENOTDIR: // Un archivo esta siendo usando como directorio
            return -404;
        case EACCES:  // El proceso no tiene permisos suficiente
            return -403;
        case EPERM: // El proceso no tiene permisos suficiente
            return -403;
        case EXDEV: // El proceso no tiene permisos suficiente
            return -403;
        case ELOOP: // usa un enlace simbolico
            return -403;    
        default:
            return -500;   
        }
    }
    return fd;
}


//--+-+-+-+-+-+-+-+-+-+-+-
int send_body_http(int fd_socket, struct file_metadata *fil){
    // manejo de errores y consulta del fstast
    sendfile(fd_socket,fil->fd,&fil->offset,4096);
}   

int get_len_file(struct file_metadata *fil){
    struct stat metadata={0};
    int status= fstat(fil->fd,&metadata);
    if(status<0){
        perror("No se puede consultar la metadad del archivo");
    }
    fil->len_file=metadata.st_size;
    if(!S_ISREG(metadata.st_mode)){
        return -404;
    }
    return 200;
}




