/*
Modulo que contiene la logica del servidor a nivel de sockets
Resolucion de dirrecion y puerto por medio de getaddrinfo
Socket TCP de cliente y del servidor no bloqueante
Socket Cliente con keep alive
*/


/* Mejorar la comunicacion de errores */
#define _POSIX_C_SOURCE 200112L
#include "include/server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <fcntl.h>
const int Q_len=10;
char IP_SERVER[]="0.0.0.0";
char SERVICE_OR_PORT[]="8080";
int passive_socket(char * ip_server, char * service_or_port, int q_len);
void get_addr_clt(struct sockaddr_in *clt_addr,char* ip_port);
void cfg_sock_clt(int fd_clt);
void cfg_sock_srv(int fd_srv);
void set_sock_opt(int fd,int level, int opt, int value, char* msg_e, int len_msg);

int passive_socket(char * ip_server, char * service_or_port, int q_len){
    // Resolucion de la direccion IP y del servicio por medio de getaddrinfo()
    int fd_sock_server=-100;
    struct addrinfo *pointer=NULL, *resolution=NULL, hints={0};
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;
    int error_addr=getaddrinfo(ip_server,service_or_port,&hints,&resolution);
    if(error_addr !=0){
        printf("Error NS:%s \n",gai_strerror(error_addr));
        fflush(stdout);
        printf("Valores Defaults IP 0.0.0.0 Puerto 8080");
        fflush(stdout);
        int error_addr_default=getaddrinfo(IP_SERVER,SERVICE_OR_PORT,&hints,&resolution);
        if(error_addr_default !=0){
            printf("Error NS:%s \n",gai_strerror(error_addr_default));
            fflush(stdout);
            return fd_sock_server;
        }

    }
    for(pointer=resolution; pointer!=NULL;pointer=pointer->ai_next){
        fd_sock_server=socket(pointer->ai_family,pointer->ai_socktype | SOCK_NONBLOCK ,pointer->ai_protocol);
        if(fd_sock_server<0){
            perror("Creacion Socket");
            continue;
        }
        cfg_sock_srv(fd_sock_server);
        int error_bd= bind(fd_sock_server,pointer->ai_addr,pointer->ai_addrlen);
        if (error_bd!=0){
            perror("Error Bind : "); 
            close(fd_sock_server);
            fd_sock_server=-100;
            continue;
        }
        int error_listen=listen(fd_sock_server,q_len);
        if(error_listen!=0){
            perror("Error Listen : "); 
            close(fd_sock_server);
            fd_sock_server=-100;
            continue;
        }
        break;
    }
    freeaddrinfo(resolution);
    return fd_sock_server;
}

void get_addr_clt(struct sockaddr_in *clt_addr,char* ip_port){
    if(inet_ntop(AF_INET,&clt_addr->sin_addr,ip_port,INET_ADDRSTRLEN)==NULL){
        perror("Error en clt ntop : "); 
    }
}

void cfg_sock_clt(int fd_clt){
    set_sock_opt(fd_clt,SOL_SOCKET,SO_KEEPALIVE,1,"SO_KEEPALIVE",sizeof("SO_KEEPALIVE"));
    set_sock_opt(fd_clt,IPPROTO_TCP,TCP_KEEPIDLE,20,"TCP_KEEPIDLE",sizeof("TCP_KEEPIDLE")); // Tiempo de inactividad  antes de enviar una peticion de keepalive
    set_sock_opt(fd_clt,IPPROTO_TCP,TCP_KEEPINTVL,5,"TCP_KEEPINTVL",sizeof("TCP_KEEPINTVL")); // Cada x tiempo se envia los keepalive 
    set_sock_opt(fd_clt,IPPROTO_TCP,TCP_KEEPCNT,4,"TCP_KEEPCNT",sizeof("TCP_KEEPCNT")); // La cantidad maxima de keepalive se envia sin tener una confirmacion
    int flags= fcntl(fd_clt,F_GETFL);
    if(flags==-1){
        perror("No se pudo obtener las flags de socket clt");
        return;
    }
    flags=flags | O_NONBLOCK;
    if(fcntl(fd_clt,F_SETFL,flags)==-1){
        perror("No se pudo configurar el fd como no bloqueante");
    }
}

void cfg_sock_srv(int fd_srv){
    set_sock_opt(fd_srv,SOL_SOCKET,SO_REUSEADDR,1,"SO_REUSEADDR",sizeof("SO_REUSEADDR"));

}

void set_sock_opt(int fd,int level, int opt, int value, char* msg_e, int len_msg){
    int error_opt= setsockopt(fd,level,opt,&value,sizeof(value));
    if(error_opt!=0){
        char msg_err[len_msg+7];
        snprintf(msg_err, sizeof(msg_err), "Error: ");
        strcat(msg_err,msg_e);
        perror(msg_err);
    }
}

