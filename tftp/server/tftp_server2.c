#include "../Common/tftp.h"

void run_server(struct tftp* my_tftp) 
{
    int n; // count bytes of stream sended or received
    int opc;
    char opcode[3];
    char fileName[20];
    char mode[10];
    char stream[SSIZE];
    FILE * fp = NULL;
    

    while(1) 
    {
        short blockNum = 0;
        // receive request 
        n = my_tftp->get_resp(my_tftp,stream);
        // parse request
        my_tftp->parse_req(fileName,mode,stream);
        // get opcode
        opc = my_tftp->get_opc(my_tftp, stream);
        
        // open file
        if(opc == RRQ)
        {   
            printf("Received[Read Request]\n");
            fp = fopen(fileName,"r");
            if(fp == NULL)
            {
                printf("%s: file not found error\n",my_tftp->progname);
                exit(4);
            }
            do
            {
                bzero(stream,SSIZE); // clear stream
                // send data
                n = my_tftp->send_dta(my_tftp,fp,stream,blockNum);
                bzero(stream,SSIZE); // clear stream
                my_tftp->get_resp(my_tftp,stream);
                // get opcode
                opc = my_tftp->get_opc(my_tftp, stream);
                // receive ack
                if(opc == ACK)
                {
                    blockNum = ntohs(*(short*)(stream+2));
                    printf("Received Ack #%d\n",blockNum);
                }
                blockNum++;
            } while (n == 516);
        }
        else if(opc == WRQ)
        {
            printf("Received[Write Request]\n");
            fp = fopen(fileName,"w");
            do
            {
                bzero(stream,SSIZE); // clear stream
                // send ack
                my_tftp->send_ack(my_tftp, stream, blockNum);
                bzero(stream,SSIZE); // clear stream
                // receive data
                n = my_tftp->get_resp(my_tftp, stream);
                // get opcode
                opc = my_tftp->get_opc(my_tftp, stream);
                if(opc == DATA)
                {
                    blockNum = ntohs(*(short*)(stream+2));
                    printf("Received Block #%d of data: %d byte(s)\n",blockNum,n);
                    fwrite(stream+4,1,n-4,fp); 
                }
                blockNum++;
            } while (n == 516);
        }  
        fclose(fp);
    }

}



int main(int argc, char *argv[])
{
    struct tftp *serv_tftp = tftp_init(argv[0]);
    
    if(argc == 3 && argv[1] == "-p"){
      serv_tftp->build_serv(serv_tftp, atoi(argv[2]));
    }
    else{
      serv_tftp->build_serv(serv_tftp, -1);
    }

    run_server(serv_tftp);

    close(serv_tftp->sockfd);
    free(serv_tftp);

    return 0;
}
