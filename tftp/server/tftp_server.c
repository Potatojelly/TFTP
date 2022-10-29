#include "../common/tftp.h"

void run_server(struct tftp* my_tftp) 
{
    struct sockaddr pcli_addr;
    int n; // count bytes of stream sended or received
    int clilen;
	char stream[SSIZE];

    while(1) 
    {
    //     clilen = sizeof(struct sockaddr);

    //     // receive a request 
    //     n = recvfrom(sockfd, stream, SSIZE, 0, &pcli_addr, &clilen);
    //     if (n < 0)
	// {
	// 	printf("%s: recvfrom error\n",progname);
	// 	exit(3);
	// }           
    //     char fileName[20];
	// 	char mode[10];
    //     char * p;
    //     short blockNum = 1;
    //     // starts parsing stream
    //     if(ntohs(*(short*) stream) == RRQ)  // recevied RRQ
    //     {
    //         printf("Received[Read Request]\n"); 
    //         p = stream + 2;
    //         strcpy(fileName,p); //get FILENAME
    //         p += strlen(fileName) + 1; // 1 is for null 
    //         strcpy(mode,p); // get MODE

    //         bzero(stream,SSIZE); // clear stream

    //         // send DATA
    //         FILE *fp = fopen(fileName,"r");
    //         if(fp == NULL)
    //         {
    //             printf("%s: file not found error\n",progname);
    //             exit(4);
    //         }


    //         *(short*) stream = htons(DATA);
    //         *(short*) (stream+2) = htons(blockNum);
    //         n = fread(stream+4,1,DATASIZE,fp) + 4;
    //         if (sendto(sockfd, stream, n, 0, &pcli_addr, clilen) != n)
    //         {
    //             printf("%s: sendto error\n",progname);
    //             exit(4);
    //         }  
    //         else
	//     {
	// 	printf("Sending Block #%d of data: %d byte(s)\n",blockNum,n);
	//     }
		   
    //         fclose(fp);
    //     } 
    //     else if(ntohs(*(short*) stream) == WRQ) // recevied WRQ
    //     {
    //         printf("Received[Write Request]\n");
    //         p = stream + 2;
    //         strcpy(fileName,p);
    //         p += strlen(fileName) + 1;
    //         strcpy(mode,p);

    //         bzero(stream,SSIZE);
            
    //         // send ACK
	//         blockNum = 0;
    //         *(short*) stream =  htons(ACK);
    //         *(short*) (stream +2) = blockNum;
    //         if (sendto(sockfd, stream, 4, 0, &pcli_addr, clilen) != 4)
    //         {
    //             printf("%s: sendto error\n",progname);
    //             exit(4);
    //         }
    //         else 
    //         {
    //             printf("Sending Ack# %d\n",blockNum);
    //         }
    //         bzero(stream,SSIZE);

    //         // receive DATA
    //         n = recvfrom(sockfd, stream, SSIZE, 0, &pcli_addr, &clilen);
	//         blockNum = ntohs(*(short*)(stream+2));
    //         if (n < 0)
    //         {
    //             printf("%s: recvfrom error\n",progname);
    //             exit(3);
    //         } 
    //         else
    //         {
    //             printf("Received Block #%d of data: %d byte(s)\n",blockNum,n);
    //         }
			
            
    //         // make a file with DATA
    //         FILE * fp = fopen(fileName,"w");
	// 	    fwrite(stream+4,1,n-4,fp);
    //         fclose(fp);
    //     }

    }

}



int main(int argc, char *argv[])
{
    struct tftp *serv_tftp = tftp_init(argv[0]);
    serv_tftp->build_serv(serv_tftp);

    run_server(serv_tftp);

    close(serv_tftp->sockfd);
    free(serv_tftp);

    return 0;
}
