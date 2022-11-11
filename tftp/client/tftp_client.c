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

        // open file
        if(strcmp("-r",opcode) == 0)
        {
            if(access(fileName,F_OK) == 0) // check if the file already exists
            {
                printf("client: file is already exist error(overwrite warning)\n");
                continue;
            }
            else
            {
                fp = fopen(fileName,"w");
            }
        }
        else if(strcmp("-w",opcode) == 0)
        {
            if(access(fileName,F_OK) == 0) // check if the file exists
            {
                if(access(fileName,R_OK) == 0)
                {
                    fp = fopen(fileName,"r");
                }
                else  // client has no permission to read the file
                {
                    printf("client: no permission to open the file error\n");
                    continue;
                }
            }
            else // client doesn't have the file to write to server
            {
                printf("client: file not found error\n");
                continue;
            }
        }

        // send request
        n = my_tftp->send_req(my_tftp, opcode, fileName, MODE, stream);
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
            else if(opc == ERROR)
            {
                short errCode = ntohs(*(short*)(stream+2));
                char errMsg[50];
                strcpy(errMsg,(stream+4));
                printf("Received ERROR #%d: %s\n",errCode,errMsg);
                fclose(fp);
                fp = NULL;
                if(errCode == 0 || errCode == 2) // not found or no permission
                {
                    remove(fileName); // remove the file created for reading from server
                }
                break;
            }
        } while (n == 516);

        if(fp != NULL)
        {
            fclose(fp);
        }
    }
}

int main(int argc, char *argv[])
{
    struct tftp *cli_tftp = tftp_init(argv[0]);
    // printf("argv[1]:%s\n",argv[1]);
    // printf("argv[2]:%s\n",argv[2]);
    // printf("argc:%d\n",argc);
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      cli_tftp->build_cli(cli_tftp, atoi(argv[2]));
    }
    else
    {
      cli_tftp->build_cli(cli_tftp, -1);
    }
    
    run_client(cli_tftp);

    close(cli_tftp->sockfd);
    free(cli_tftp);
    return 0;
}
