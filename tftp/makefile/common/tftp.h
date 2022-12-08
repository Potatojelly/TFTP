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
#include <signal.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define SERV_UDP_PORT 61124
#define SERV_HOST_ADDR "127.0.0.1" //"10.158.82.32"  

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#define SERVER 1
#define CLIENT 2

#define SSIZE 600                  // Stream Size
#define MODE "octet"

#define TIMEOUT 2

int count;

struct tftp
{
    void (*build_serv)(struct tftp *my_tftp, int port);
    void (*build_servThread)(struct tftp *my_tftp);
    void (*build_cli)(struct tftp *my_tftp, int port);
    void (*parse_req)(char* fname, char* mode, char* packet);
    int (*send_req)(struct tftp *my_tftp, int opcode, char* fname, char* mode, char* packet);
    int (*send_dta)(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum);
    int (*resend_dta)(struct tftp *my_tftp, char* packet,short blockNum);
    int (*send_ack)(struct tftp *my_tftp, char* packet, short blockNum);
    int (*send_err)(struct tftp *my_tftp, char* packet, short errCode);
    int (*get_resp)(struct tftp *my_tftp, char* packet);
    int (*get_opc)(struct tftp *my_tftp, char* packet);
    void (*retxREQ)(struct tftp *my_tftp);
    int (*retxDATA)(struct tftp *my_tftp);
    int (*retxACK)(struct tftp *my_tftp);
    void (*handle_timeout)(int signal);
    int (*register_handler)(struct tftp *my_tftp);
    void (*timeout_reset)();

    char* progname;
    FILE * fp;

    int sockfd;
    int clilen;
    int servlen;
    int type;
    int recByte; 
    int snByte;
    int blockNum; 
    int opcode;

    struct sockaddr_in cli_addr;
    struct sockaddr_in serv_addr;

    char outStream[SSIZE];
    char inStream[SSIZE];
    char fileName[20];
    char mode[10];
};

struct tftp* tftp_init(char* name);
void tftp_free(struct tftp *my_tftp);
void build_servSocket(struct tftp *my_tftp, int port);
void build_servThreadSocket(struct tftp *my_tftp);
void build_cliSocket(struct tftp *my_tftp, int port);
void parse_request(char* fname, char* mode, char* packet);
int send_request(struct tftp *my_tftp, int opcode, char* fname, char* mode, char* packet); 
int send_data(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum);
int resend_data(struct tftp *my_tftp, char* packet,short blockNum);
int send_ack(struct tftp *my_tftp, char* packet, short blockNum);
int send_error(struct tftp *my_tftp, char* packet, short errCode);
int get_response(struct tftp *my_tftp, char* packet);
int get_opcode(struct tftp *my_tftp, char* packet);
void retxREQ(struct tftp *my_tftp);
int retxDATA(struct tftp *my_tftp);
int retxACK(struct tftp *my_tftp);
void handle_timeout(int signal);
int register_handler(struct tftp *my_tftp);
void timeout_reset();
