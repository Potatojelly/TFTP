#include "../common/tftp.h"

void run_client(struct tftp *my_tftp)
{
    int n; // count bytes of stream sended or received
    int opc;
    char opcode[3];
    char fileName[20];
    char stream[SSIZE];
    FILE * fp = NULL;


    while(1)
    {
        short blockNum = 0;
        // receive input 
        scanf("%s" "%s", opcode,fileName);

        // send request
        n = my_tftp->send_req(my_tftp, opcode, fileName, MODE, stream);
        // open file
        opc = my_tftp->get_opc(my_tftp, stream);
        if(opc == RRQ)
        {
            fp = fopen(fileName,"w");a
        }
        else if(opc == WRQ)
        {
            fp = fopen(fileName,"r");
            if(fp == NULL)
            {
                printf("%s: file not found error\n",my_tftp->progname);
                exit(4);
            }
        }

        bzero(&stream,SSIZE);
        
        do
        {
            // get response
            n = my_tftp->get_resp(my_tftp, stream);
            // get opcode
            opc = my_tftp->get_opc(my_tftp, stream);
            if(opc == DATA)
            {
                // make a file with DATA
                blockNum = ntohs(*(short*)(stream+2));
                printf("Received Block #%d of data: %d byte(s)\n",blockNum,n);
                fwrite(stream+4,1,n-4,fp); 

                bzero(&stream,SSIZE); // clear stream
                //send ACK
                my_tftp->send_ack(my_tftp, stream, blockNum);
                bzero(&stream,SSIZE); // clear stream
            }
            else if(opc == ACK)
            {
                blockNum = ntohs(*(short*)(stream+2));
                printf("Received Ack #%d\n",blockNum);
                bzero(&stream,SSIZE); // clear stream
                n = my_tftp->send_dta(my_tftp,fp,stream,blockNum);
                bzero(&stream,SSIZE); // clear stream
            }
        } while (n == 516);

        fclose(fp);
    }
}

int main(int argc, char *argv[])
{
    struct tftp *cli_tftp = tftp_init(argv[0]);
    
    if(argc == 3 && argv[1] == "-p"){
      cli_tftp->build_cli(cli_tftp, atoi(argv[2]));
    }
    else{
      cli_tftp->build_cli(cli_tftp, -1);
    }
    
    
    run_client(cli_tftp);

    close(cli_tftp->sockfd);
    free(cli_tftp);
    return 0;
}
