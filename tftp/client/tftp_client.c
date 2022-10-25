#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close socket
#include <arpa/inet.h> //for inet_addr()

#define SERV_UDP_PORT 61123 
#define SERV_HOST_ADDR "127.0.0.1" //"10.158.82.32" 

// opcodes
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#define SSIZE 600
#define MODE "octet"

char * progname;

void tftp_cli(int sockfd, struct sockaddr *pserv_addr, int servlen)
{
    int n; // count bytes of stream sended or received
    char ops[3];
    char fileName[20];
    char stream[SSIZE];
    char *p;

    while(1)
    {
        // receive a command 
        scanf("%s" "%s", ops,fileName);

        // get the opcode from the command
		if(ops[1] == 'r')
		{
			*(short*)stream = htons(RRQ);
		} 
		else if (ops[1] == 'w')
		{
			*(short*)stream = htons(WRQ);
		}
        // get the filename from the command
        p = stream + 2;
        strcpy(p,fileName);
        p += strlen(fileName) + 1; // 1 is for null
        strcpy(p,MODE);
        p += strlen(MODE) + 1;

        n = p - stream;

        // send a request
        if(sendto(sockfd,stream,n,0,pserv_addr,servlen) != n)
        {
            printf("%s: sendto error on socket\n",progname);
			exit(3);
        }

        bzero(&stream,SSIZE);
        
        // receive a response
        n =  recvfrom(sockfd,stream,SSIZE,0,NULL,NULL);
        if (n < 0)
        {
            printf("%s: recvfrom error\n",progname);
            exit(4);
        }
        
       short blockNum = 1;
       // check opcode
       if(ntohs(*(short*) stream) == DATA) // receive DATA
       {
            // make a file with DATA
	        *(short*) (stream +2) = htons(blockNum);   
            printf("Received Block #%d of data: %d byte(s)\n",blockNum,n);
            FILE * fp = fopen(fileName,"w");
            fwrite(stream+4,1,n-4,fp); // write 512 bytes
            fclose(fp);
       } 
       else if(ntohs(*(short*) stream) == ACK) //receive ACK
       {    
            *(short*) (stream +2) = htons(blockNum);    
            printf("Received Ack #%d\n",blockNum);
            bzero(&stream,SSIZE);
            FILE *fp = fopen(fileName,"r");
            if(fp == NULL)
            {
                printf("%s: file not found error\n",progname);
                exit(4);
            }
            // send DATA
            *(short*) stream = htons(DATA);
            *(short*) (stream+2) = htons(blockNum);
            n = fread(stream+4,1,512,fp) + 4; // 4byte is for opcode and block number
            if(sendto(sockfd,stream,n,0,pserv_addr,servlen) != n)
            {
                printf("%s: sendto error on socket\n",progname);
                exit(3);
            }   
            else
            {
            printf("Sending Block #%d of data: %d byte(s)\n",blockNum,n);
            }
            fclose(fp); 
       }

    }
}

int main(int argc, char *argv[])
{
    int sockfd;

    struct sockaddr_in cli_addr;
    struct sockaddr_in serv_addr;

    progname = argv[0];

    // server socket
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port = htons(SERV_UDP_PORT);

    if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
       	printf("%s: can't open datagram socket\n",progname);
		exit(1);
    }

    // clinet socket
    bzero((char *) &cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons(0);

    if(bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
    {
		printf("%s: can't bind local address\n",progname);
		exit(2);
    }

    tftp_cli(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    close(sockfd);
    exit(0);
    return 0;
}
