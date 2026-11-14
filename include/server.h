#ifndef SERVER_H
#define SERVER_H
#include "http.h"
#include <netinet/in.h>


int passive_socket(char * ip_server, char * service_or_port, int q_len);
void get_addr_clt(struct sockaddr_in *clt_addr,char* ip_port);
void cfg_sock_clt(int fd_clt);
void cfg_sock_srv(int fd_srv);
void set_sock_opt(int fd,int level, int opt, int value, char* msg_e, int len_msg);
void server();
#endif