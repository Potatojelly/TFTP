#include "../common/tftp.h"

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
            if(access(fileName,F_OK) == 0) // check if the file exists
            {
                if(access(fileName,R_OK) == 0)
                {
                    fp = fopen(fileName,"r");
                }
                else  // server has no permission to read the file
                {
                    bzero(stream,SSIZE); // clear stream
                    my_tftp->send_err(my_tftp,stream,2);
                    continue;
                }
            }
            else  // server doesn't have the requested file from client 
            {
                bzero(stream,SSIZE); // clear stream
                my_tftp->send_err(my_tftp,stream,0);
                continue;
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
            if(access(fileName,F_OK) == 0) // check if the file exists
            {
                bzero(stream,SSIZE); 
                my_tftp->send_err(my_tftp,stream,1);
                continue;
            }
            else
            {
                fp = fopen(fileName,"w");
            }

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
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      serv_tftp->build_serv(serv_tftp, atoi(argv[2]));
    }
    else
    {
      serv_tftp->build_serv(serv_tftp, -1);
    }

    run_server(serv_tftp);

    close(serv_tftp->sockfd);
    free(serv_tftp);

    return 0;
}
