#pragma once
#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>     

#define SERV_UDP_PORT 61124
#define SERV_HOST_ADDR "127.0.0.1" //"10.158.82.32"  

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#define SERVER 1
#define CLIENT 2

#define SSIZE 600
#define MODE "octet"

struct tftp
{
    void (*build_serv)(struct tftp *my_tftp, int port);
    void (*build_cli)(struct tftp *my_tftp, int port);
    void (*parse_req)(char* fname, char* mode, char* packet);
    int (*send_req)(struct tftp *my_tftp, char* opc, char* fname, char* mode, char* packet);
    int (*send_dta)(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum);
    int (*send_ack)(struct tftp *my_tftp, char* packet, short blockNum);
    int (*get_resp)(struct tftp *my_tftp, char* packet);
    int (*get_opc)(struct tftp *my_tftp, char* packet);
    
    int sockfd;
    struct sockaddr_in cli_addr;
    int clilen;
    struct sockaddr_in serv_addr;
    int servlen;
    int type;
    char* progname;
};

struct tftp* tftp_init(char* name);
void build_servSocket(struct tftp *my_tftp, int port);
void build_cliSocket(struct tftp *my_tftp, int port);
void parse_request(char* fname, char* mode, char* packet);
int send_request(struct tftp *my_tftp, char* opc, char* fname, char* mode, char* packet); 
int send_data(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum);
int send_ack(struct tftp *my_tftp, char* packet, short blockNum);
int get_response(struct tftp *my_tftp, char* packet);
int get_opcode(struct tftp *my_tftp, char* packet);
