#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close socket

#define SERV_UDP_PORT 61123 

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#define SSIZE 600
#define MODE "octet"

char * progname;

void tftp_serv(int sockfd) 
{
    struct sockaddr pcli_addr;
    int n; // count bytes of stream sended or received
    int clilen;
	char stream[SSIZE];

    while(1) 
    {
        clilen = sizeof(struct sockaddr);

        // receive a request 
        n = recvfrom(sockfd, stream, SSIZE, 0, &pcli_addr, &clilen);
        if (n < 0)
	{
		printf("%s: recvfrom error\n",progname);
		exit(3);
	}           
        char fileName[20];
		char mode[10];
        char * p;
        
        // starts parsing stream
        if(ntohs(*(short*) stream) == RRQ)  
        {
            printf("Received[Read Request]\n"); 
            p = stream + 2;
            strcpy(fileName,p); //get FILENAME
            p += strlen(fileName) + 1; // 1 is for null 
            strcpy(mode,p); // get MODE

            bzero(stream,SSIZE); // clear stream
			
            int blockNum = 0;

        // send DATA
            FILE *fp = fopen(fileName,"r");
            if(fp == NULL)
            {
                printf("%s: file not found error\n",progname);
                exit(4);
            }


            *(short*) stream = htons(DATA);
            *(short*) (stream+2) = blockNum;
            n = fread(stream+4,1,512,fp) + 4;
            if (sendto(sockfd, stream, n, 0, &pcli_addr, clilen) != n)
            {
                printf("%s: sendto error\n",progname);
                exit(4);
            }  
            fclose(fp);
        } 
        else if(ntohs(*(short*) stream) == WRQ) // 
        {
            printf("Received[Write Request]\n");
            p = stream + 2;
            strcpy(fileName,p);
            p += strlen(fileName) + 1;
            strcpy(mode,p);

            int blockNum = 0;

            bzero(stream,SSIZE);
            
            // send ACK
            *(short*) stream =  htons(ACK);
            *(short*) (stream +2) = blockNum;
            if (sendto(sockfd, stream, 4, 0, &pcli_addr, clilen) != 4)
            {
                printf("%s: sendto error\n",progname);
                exit(4);
            }
            else 
            {
                printf("Sending Ack# %d\n",blockNum);
            }
             bzero(stream,SSIZE);

            // receive DATA
            n = recvfrom(sockfd, stream, SSIZE, 0, &pcli_addr, &clilen);
	    blockNum = ntohs(*(short*)(stream+2));
            if (n < 0)
            {
                printf("%s: recvfrom error\n",progname);
                exit(3);
            } 
	    else
	    {
		printf("Received Block #%d! of data: %d byte(s)\n",blockNum,n);
	    }
			
            
            // make a file with DATA
            FILE * fp = fopen(fileName,"w");
		    fwrite(stream+4,1,n-4,fp);
            fclose(fp);
        }

               
    }

}



int main(int argc, char *argv[])
{
    int sockfd;

    struct sockaddr_in serv_addr;

    progname = argv[0];

    if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
       	printf("%s: can't open datagram socket\n",progname);
		exit(1);
    }

    // server socket
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_UDP_PORT);


    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
		printf("%s: can't bind local address\n",progname);
		exit(2);
    }

    tftp_serv(sockfd);

    close(sockfd);
    exit(0);
    return 0;
}
