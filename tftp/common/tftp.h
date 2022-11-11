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

// // implementation
// struct tftp* tftp_init(char* name) 
// {
// 	struct tftp *my_tftp = malloc(sizeof(struct tftp));

//   my_tftp->build_cli = build_cliSocket;
//   my_tftp->build_serv = build_servSocket;
//   my_tftp->parse_req = parse_request;
//   my_tftp->send_req = send_request;
//   my_tftp->send_dta = send_data;
//   my_tftp->send_ack = send_ack;
//   my_tftp->get_resp = get_response;
//   my_tftp->get_opc = get_opcode;

//   my_tftp->sockfd = 0;
//   my_tftp->clilen = sizeof(my_tftp->cli_addr);
//   my_tftp->servlen = sizeof(my_tftp->serv_addr);
//   my_tftp->progname = name;
//   my_tftp->type = 0;

// 	return my_tftp;
// }

// void build_servSocket(struct tftp *my_tftp)
// {
//     my_tftp->type = SERVER;
//     if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
//     {
//        	printf("%s: can't open datagram socket\n",*(my_tftp->progname));
// 		exit(1);
//     }
//     bzero((char *) &(my_tftp->cli_addr), sizeof(my_tftp->cli_addr));
//     my_tftp->cli_addr.sin_family = AF_INET;
//     my_tftp->cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     my_tftp->cli_addr.sin_port = htons(SERV_UDP_PORT);
        
//     if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->cli_addr), my_tftp->clilen) < 0)
//     {
// 		printf("%s: can't bind local address\n",my_tftp->progname);
// 		exit(2);
//     }
// }

// void build_cliSocket(struct tftp *my_tftp)
// {
//     my_tftp->type = CLIENT;
//     if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
//     {
//        	printf("%s: can't open datagram socket\n",*(my_tftp->progname));
// 		exit(1);
//     }
//     // cli addr
//     bzero((char *) &my_tftp->cli_addr, sizeof(my_tftp->cli_addr));
//     my_tftp->cli_addr.sin_family = AF_INET;
//     my_tftp->cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     my_tftp->cli_addr.sin_port = htons(0);
//     // serv addr
//     bzero((char *) &(my_tftp->serv_addr), sizeof(my_tftp->serv_addr));
//     my_tftp->serv_addr.sin_family = AF_INET;
//     my_tftp->serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
//     my_tftp->serv_addr.sin_port = htons(SERV_UDP_PORT);
        
//     if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->cli_addr), my_tftp->clilen) < 0)
//     {
// 		printf("%s: can't bind local address\n",my_tftp->progname);
// 		exit(2);
//     }
// }

// void parse_request(char* fname, char* mode, char* packet)
// {
//   char *p;
//   p = packet + 2;
//   strcpy(fname,p); //get FILENAME
//   p += strlen(fname) + 1; // 1 is for null 
//   strcpy(mode,p); // get MODE
// }

// // returns how many bytes it sends
// int send_request(struct tftp *my_tftp, char* opc, char* fname, char* mode, char* packet ) 
// {
//     int n;
//     char *p; 
//     // get opcode 
//     if(opc[1] == 'r')
//     {
//         *(short*)packet = htons(RRQ);
//     } 
//     else if (opc[1] == 'w')
//     {
//         *(short*)packet = htons(WRQ);
//     }
//     // get filename 
//     p = packet + 2;
//     strcpy(p,fname);
//     p += strlen(fname) + 1; // 1 is for null
//     strcpy(p,mode);
//     p += strlen(mode) + 1;

//     n = p - packet;
//     // send packet
//     if(sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *) &(my_tftp->serv_addr),my_tftp->servlen) != n)
//     {
//         printf("%s: sendto error on socket\n",my_tftp->progname);
//         exit(3);
//     }
//     return n;
// }

// int get_response(struct tftp *my_tftp, char* packet)
// {
//   int n;
//   if(my_tftp->type == SERVER)
//   {
//     n =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->cli_addr),&(my_tftp->clilen));
//   }
//   else if(my_tftp->type == CLIENT)
//   {
//     n =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->serv_addr),&(my_tftp->servlen));
//   }

//   if (n < 0)
//   {
//       printf("%s: recvfrom error\n",my_tftp->progname);
//       exit(4);
//   }
//   return n;
// }

// int get_opcode(struct tftp *my_tftp, char* packet)
// {
//   int opcode;
//   if(ntohs(*(short*) packet) == RRQ)
//   {
//     opcode = RRQ;
//   }
//   else if(ntohs(*(short*) packet) == WRQ)
//   {
//     opcode = WRQ;
//   }
//   else if(ntohs(*(short*) packet) == DATA)
//   {
//     opcode = DATA;
//   }
//   else if(ntohs(*(short*) packet) == ACK)
//   {
//     opcode = ACK;
//   }
//     else if(ntohs(*(short*) packet) == ERROR)
//   {
//     opcode = ERROR;
//   }
//   return opcode;
// }

// int send_data(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum)
// {
//     int n;
//     int sn;
//     *(short*) packet =  htons(DATA);
//     *(short*) (packet +2) = htons(blockNum);
//     n = fread(packet+4,1,512,fp) + 4; // 4byte is for opcode and block number
//     if(my_tftp->type == SERVER)
//     {
//       sn = sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
//     }
//     else if(my_tftp->type == CLIENT)
//     {
//       sn = sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
//     }    
//     if(sn != n)
//     {
//         printf("%s: sendto error on socket\n",my_tftp->progname);
//         exit(3);            
//     }
//     else
//     {
//         printf("Sending block #%d of data\n",blockNum);
//     }
//     return sn;
// }

// int send_ack(struct tftp *my_tftp, char* packet, short blockNum)
// {
//   int n;
//   *(short*) packet =  htons(ACK);
//   *(short*) (packet +2) = htons(blockNum);

//   if(my_tftp->type == SERVER)
//   {
//     n = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
//   }
//   else if(my_tftp->type == CLIENT)
//   {
//     n = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
//   }

//   if(n != 4)
//   {
//       printf("%s: sendto error on socket\n",my_tftp->progname);
//       exit(3);            
//   }
//   else
//   {
//       printf("Sending Ack# %d\n",blockNum);
//   }
//   return n;
// }
