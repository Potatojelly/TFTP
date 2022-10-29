#include "../common/tftp.h"

void run_client(struct tftp *my_tftp)
{
    int n; // count bytes of stream sended or received
    char ops[3];
    char fileName[20];
    char stream[SSIZE];

    /*
        Needs to write codes again uisng tftp
    */
    while(1)
    {
    //     // receive a command 
    //     scanf("%s" "%s", ops,fileName);

    //     // get the opcode from the command
	// 	if(ops[1] == 'r')
	// 	{
	// 		*(short*)stream = htons(RRQ);
	// 	} 
	// 	else if (ops[1] == 'w')
	// 	{
	// 		*(short*)stream = htons(WRQ);
	// 	}
    //     // get the filename from the command
    //     p = stream + 2;
    //     strcpy(p,fileName);
    //     p += strlen(fileName) + 1; // 1 is for null
    //     strcpy(p,MODE);
    //     p += strlen(MODE) + 1;

    //     n = p - stream;

    //     // send a request
    //     if(sendto(sockfd,stream,n,0,pserv_addr,servlen) != n)
    //     {
    //         printf("%s: sendto error on socket\n",progname);
	// 		exit(3);
    //     }

    //     bzero(&stream,SSIZE);
        
    //     // receive a response
    //     n =  recvfrom(sockfd,stream,SSIZE,0,NULL,NULL);
    //     if (n < 0)
    //     {
    //         printf("%s: recvfrom error\n",progname);
    //         exit(4);
    //     }
        
    //    short blockNum = 1;
    //    // check opcode
    //    if(ntohs(*(short*) stream) == DATA) // receive DATA
    //    {
    //         // make a file with DATA
	//         *(short*) (stream +2) = htons(blockNum);   
    //         printf("Received Block #%d of data: %d byte(s)\n",blockNum,n);
    //         FILE * fp = fopen(fileName,"w");
    //         fwrite(stream+4,1,n-4,fp); // write 512 bytes
    //         fclose(fp);
    //    } 
    //    else if(ntohs(*(short*) stream) == ACK) //receive ACK
    //    {    
    //         blockNum = ntohs(*(short*) (stream+2));  
    //         printf("Received Ack #%d\n",blockNum);
    //         bzero(&stream,SSIZE);
    //         FILE *fp = fopen(fileName,"r");
    //         if(fp == NULL)
    //         {
    //             printf("%s: file not found error\n",progname);
    //             exit(4);
    //         }
    //         // send DATA
    //         blockNum++;
    //         *(short*) stream = htons(DATA);
    //         *(short*) (stream+2) = htons(blockNum);
    //         n = fread(stream+4,1,512,fp) + 4; // 4byte is for opcode and block number
    //         if(sendto(sockfd,stream,n,0,pserv_addr,servlen) != n)
    //         {
    //             printf("%s: sendto error on socket\n",progname);
    //             exit(3);
    //         }   
    //         else
    //         {
    //         printf("Sending Block #%d of data: %d byte(s)\n",blockNum,n);
    //         }
    //         fclose(fp); 
    //    }

    }
}

int main(int argc, char *argv[])
{
    struct tftp *cli_tftp = tftp_init(argv[0]);
    cli_tftp->build_cli(cli_tftp);

    run_client(cli_tftp);

    close(cli_tftp->sockfd);
    free(cli_tftp);
    return 0;
}
