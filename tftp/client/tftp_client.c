#include "../common/tftp.h"

#include <stdio.h>
#include <stdbool.h>

struct tftp* cli_tftp;
int count = 0;
int recByte = 0; 
int snByte = 0;
int opc;
char opcode[3];
char fileName[20];
char outStream[SSIZE];
char inStream[SSIZE];
FILE * fp = NULL;
short blockNum = 0;

// // alarm hanlder
// void retxDATA(int signal)
// {
//     count++;
//     if(count <= 10)
//     {
//         snByte = cli_tftp->send_dta(cli_tftp,fp,outStream,blockNum); 
//         alarm(2); 
//     }
//     else
//     {
//         printf("client: 10 consecutive timesouts has occurd.\n");
//         fclose(fp);
//         exit(0);
//     }
// }

// // alarm hanlder
// void retxACK(int signal)
// {
//     count++;
//     if(count <= 10)
//     {
//         cli_tftp->send_ack(cli_tftp, outStream, blockNum);
//         alarm(2); // alarm
//     }
//     else
//     {
//         printf("client: 10 consecutive timesouts has occurd.\n");
//         fclose(fp);
//         exit(0);
//     }
// }

// // alarm hanlder
// void retxREQ(int signal)
// {
//     count++;
//     if(count <= 10)
//     {
//         snByte = cli_tftp->send_req(cli_tftp, opcode, fileName, MODE, outStream);
//         alarm(2); // alarm
//     }
//     else
//     {
//         printf("client: 10 consecutive timesouts has occurd.\n");
//         fclose(fp);
//         exit(0);
//     }
// }

void run_client()
{
    while(1)
    {
        blockNum = 0;
        // receive input 
        scanf("%s" "%s", opcode,fileName);

        if(strcmp("-r",opcode) == 0)                                                                    // request reading
        {
            // open file
            if(access(fileName,F_OK) == 0)                                                              // check if the file already exists
            {
                printf("client: file is already exist error(overwrite warning)\n");
                continue;
            }
            else
            {
                fp = fopen(fileName,"w");
                // send RRQ 
                bzero(&outStream,SSIZE);                                                                    
                snByte= cli_tftp->send_req(cli_tftp, opcode, fileName, MODE, outStream);
            }

            do
            {
                // receive response
                bzero(&inStream,SSIZE);
                recByte = cli_tftp->get_resp(cli_tftp, inStream);

                // get opcode
                opc = cli_tftp->get_opc(cli_tftp, inStream);
                if(opc == DATA)
                {
                    blockNum = ntohs(*(short*)(inStream+2));
                    printf("Received Block #%d of data: %d byte(s)\n",blockNum,recByte);
                    fwrite(inStream+4,1,recByte-4,fp);                                                        // make a file with received DATA from server

                    //send ACK
                    bzero(&outStream,SSIZE); 
                    snByte = cli_tftp->send_ack(cli_tftp, outStream, blockNum);
                }
                else if(opc == ERROR)
                {
                    short errCode = ntohs(*(short*)(inStream+2));
                    char errMsg[50];
                    strcpy(errMsg,(inStream+4));
                    printf("Received ERROR #%d: %s\n",errCode,errMsg);
                    fclose(fp);
                    fp = NULL;
                    if(errCode == 0 || errCode == 2)                                                     // not found or no permission
                    {
                        remove(fileName);                                                                // remove the file created for reading from server
                    }
                    break;
                }
            } while (recByte == 516);                                                                    // if received DATA from server < 516, escape the loop

        }
        else if(strcmp("-w",opcode) == 0)                                                                // request writing 
        {
            // open file
            if(access(fileName,F_OK) == 0)                                                               // check if the file exists
            {
                if(access(fileName,R_OK) == 0)
                {
                    fp = fopen(fileName,"r");
                }
                else                                                                                     // client has no permission to read the file
                {
                    printf("client: no permission to open the file error\n");
                    continue;
                }
            }
            else                                                                                         // client doesn't have the file to write to server
            {
                printf("client: file not found error\n");
                continue;
            }
            bool WRQCheck = false;
            do
            {
                if(!WRQCheck)
                {
                    // send WRQ 
                    bzero(&outStream,SSIZE); 
                    cli_tftp->send_req(cli_tftp, opcode, fileName, MODE, outStream);
                    WRQCheck = true;
                    snByte = 516;                                                                        // for not escaping the loop
                }
                else
                {
                    // send DATA 
                    bzero(&outStream,SSIZE);   
                    snByte = cli_tftp->send_dta(cli_tftp,fp,outStream,blockNum);
                }

                // receive response
                bzero(&inStream,SSIZE); 
                recByte = cli_tftp->get_resp(cli_tftp, inStream);

                // get opcode
                opc = cli_tftp->get_opc(cli_tftp, inStream);
                if(opc == ACK)
                {
                    blockNum = ntohs(*(short*)(inStream+2));
                    printf("Received Ack #%d\n",blockNum);
                }
                else if(opc == ERROR)
                {
                    short errCode = ntohs(*(short*)(inStream+2));
                    char errMsg[50];
                    strcpy(errMsg,(inStream+4));
                    printf("Received ERROR #%d: %s\n",errCode,errMsg);
                    fclose(fp);
                    fp = NULL;
                    break;
                }
            } while (snByte == 516);
        }

        if(fp != NULL)
        {
            fclose(fp);
            fp = NULL;
        }
    }
}

int main(int argc, char *argv[])
{
    cli_tftp = tftp_init(argv[0]);
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
