/*
Modulo que contiene la logica del servidor a nivel de sockets
Resolucion de dirrecion y puerto por medio de getaddrinfo
Servidor TCP con una cola definida
*/
#define _POSIX_C_SOURCE 200112L
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
const int Q_len=10;
char IP_SERVER[]="0.0.0.0";
char SERVICE_OR_PORT[]="9255";

int passive_socket(char * ip_server, char * service_or_port, int q_len){
    // Resolucion de la direccion IP y del servicio por medio de getaddrinfo()
    int fd_sock_server=-1;
    struct addrinfo *pointer, *resolution;
    struct addrinfo hints={0};
    hints.ai_family=AF_INET; // Se puede usar AF_UNSPEC para que se pueda decidir con IPV4 y IPv6
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;
    int error_addrinfo=getaddrinfo(ip_server,service_or_port,&hints,&resolution);
    if(error_addrinfo !=0){
        printf("Error al buscar el service");
    }
    for(pointer=resolution; pointer!=NULL;pointer=pointer->ai_next){
        fd_sock_server=socket(pointer->ai_family,pointer->ai_socktype,pointer->ai_protocol);
        if(fd_sock_server<0){
            continue;
        }
        int error_bind= bind(fd_sock_server,pointer->ai_addr,pointer->ai_addrlen);
        if (error_bind!=0){
            printf("Error al realizar la asociacion del socket con la interfaz");
            close(fd_sock_server);
            continue;
        }
        break;
    }
    int error_listen=listen(fd_sock_server,q_len);
    if(error_listen!=0){
        printf("Error al colocar al socket de forma pasiva");
        close(fd_sock_server);
        fd_sock_server=-1;
    }
    freeaddrinfo(resolution);
    return fd_sock_server;
}

int get_addr_clt(struct sockaddr_in *clt_addr,char* ip_port,int len_ip_port){
    if(len_ip_port<INET_ADDRSTRLEN){
        return -1;
    }
    if(inet_ntop(AF_INET,&clt_addr->sin_addr,ip_port,len_ip_port)==NULL){
        printf("Error al transformar la direccion IP");
        return -1;
    }
    return 0;
}

void server_logic(void){
    printf("server ----");
    fflush(stdout);
    int fd_sock_server=-1;
    struct sockaddr_in cltaddr={0};
    fd_sock_server=passive_socket(IP_SERVER,SERVICE_OR_PORT,Q_len);
    socklen_t len_cltaddr = sizeof(cltaddr);
    if(fd_sock_server<0){
        printf("No se pudo crear el socket de forma adecuada");
        return;
    }
    printf("server ----");
    fflush(stdout);
    while(1){
        int new_socket_client=accept(fd_sock_server,(struct sockaddr*)&cltaddr,&len_cltaddr);
        char ip_clt_port[INET_ADDRSTRLEN];
        int error=get_addr_clt(&cltaddr,ip_clt_port,sizeof(ip_clt_port));
        printf("Cliente conectado: %s  port: %d",ip_clt_port,ntohs(cltaddr.sin_port));
        // Tomar el nombre del cliente para los logs
    }

}

int main(){
    printf("resr");
    server_logic();
}