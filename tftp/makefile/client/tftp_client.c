#include "../common/tftp.h"

struct tftp* cli_tftp;
char* opc;
char* fileName;

void run_client()
{
    // set opcode and filename from user input
    strcpy(cli_tftp->fileName,fileName);
    if(strcmp("-r",opc) == 0)
    {
        cli_tftp->opcode = RRQ;
    }
    else if(strcmp("-w",opc) == 0)
    {
        cli_tftp->opcode = WRQ;
    }

    cli_tftp->blockNum = 0;
    // request RRQ
    if(cli_tftp->opcode == RRQ)                                                                               
    {
        // open file
        if(access(cli_tftp->fileName,F_OK) == 0)                                                                // check if the file already exists
        {
            printf("client: file is already exist error(overwrite warning)\n");
            exit(0);
        }
        else
        {
            cli_tftp->fp = fopen(cli_tftp->fileName,"w");
            // send RRQ 
            bzero(cli_tftp->outStream,SSIZE);                                                                    
            cli_tftp->snByte= cli_tftp->send_req(cli_tftp, cli_tftp->opcode, cli_tftp->fileName, MODE, cli_tftp->outStream);
            alarm(TIMEOUT);
        }

        do
        {
            while(1)
            {
                // wait response
                bzero(cli_tftp->inStream,SSIZE);
                cli_tftp->recByte = cli_tftp->get_resp(cli_tftp, cli_tftp->inStream);
        
                if(cli_tftp->recByte < 0)
                {
                    if(errno = EINTR)
                    {
                        if(cli_tftp->blockNum == 0)
                        {
                            cli_tftp->retxREQ(cli_tftp);                                                        // retransmit REQ when failed to receive DATA #1
                        }
                        else
                        {
                            cli_tftp->retxACK(cli_tftp);                                                        // retransmit ACK when failed to receive the corresponding DATA 
                        }
                    }
                    else
                    {
                        exit(0);
                    }
                }
                else
                {
                    // extract opcode from the response
                    cli_tftp->opcode = cli_tftp->get_opc(cli_tftp, cli_tftp->inStream);
                    cli_tftp->timeout_reset();
                    if(cli_tftp->opcode == RRQ)
                    {
                        cli_tftp->blockNum++;                                                                       // set blockNum = 1 to receive DATA #1
                    }
                }
                // when the reponse is DATA
                if(cli_tftp->opcode == DATA)
                {
                    if(cli_tftp->blockNum > ntohs(*(short*)(cli_tftp->inStream+2)))                                  // discard duplicated DATA
                    {
                        alarm(TIMEOUT);
                        continue;
                    }
                    cli_tftp->blockNum = ntohs(*(short*)(cli_tftp->inStream+2));
                    printf("Received Block #%d of data: %d byte(s)\n",cli_tftp->blockNum,(cli_tftp->recByte-4));
                    fwrite(cli_tftp->inStream+4,1,cli_tftp->recByte-4,cli_tftp->fp);                                 // make a file with received DATA 
                    break;
                }
                // when the reponse is ERROR
                else if(cli_tftp->opcode == ERROR)
                {
                    short errCode = ntohs(*(short*)(cli_tftp->inStream+2));
                    char errMsg[50];
                    strcpy(errMsg,(cli_tftp->inStream+4));
                    printf("Received ERROR #%d: %s\n",errCode,errMsg);
                    fclose(cli_tftp->fp);
                    cli_tftp->fp = NULL;
                    if(errCode == 0 || errCode == 2)                                                                 // not found or no permission
                    {
                        remove(cli_tftp->fileName);                                                                  // remove the file created to read from server
                    }
                    exit(0);
                }
            }
            // TIMEOUT CASE2 TEST  - server doesn't receive ACK and keeps sending DATA
            // if(cli_tftp->blockNum == 30)
            // {
            //     sleep(8);
            // }

            //send ACK
            bzero(cli_tftp->outStream,SSIZE); 
            cli_tftp->snByte = cli_tftp->send_ack(cli_tftp, cli_tftp->outStream, cli_tftp->blockNum);
            if(cli_tftp->recByte == 516)                                                                            // don't set alarm after sending ACK when all packets are received
            {
                alarm(TIMEOUT);
            }
            cli_tftp->blockNum++;
            
        } while (cli_tftp->recByte == 516);                                                                         // terminate when all contents of the file has received

    }
    // request WRQ
    else if(cli_tftp->opcode == WRQ)                                                               
    {
        // open file
        if(access(cli_tftp->fileName,F_OK) == 0)                                                                 // check if the file exists
        {
            if(access(cli_tftp->fileName,R_OK) == 0)                                                             // check if the file can be read
            {
                cli_tftp->fp = fopen(cli_tftp->fileName,"r");
            }
            else                                                                                                 // client has no permission to read the file
            {
                printf("client: no permission to open the file error\n");
                exit(0);
            }
        }
        else                                                                                                     // client doesn't have the file to write to server
        {
            printf("client: file not found error\n");
            exit(0);
        }
        // bool variable to send DATA after sending WRQ
        bool WRQCheck = false;
        do
        {
            if(!WRQCheck)
            {
                // send WRQ 
                bzero(cli_tftp->outStream,SSIZE); 
                cli_tftp->send_req(cli_tftp, cli_tftp->opcode, cli_tftp->fileName, MODE, cli_tftp->outStream);
                WRQCheck = true;
                cli_tftp->snByte = 516;                                                                           // set 516 to avoid to escape the loop
                alarm(TIMEOUT);
            }
            else
            {
                // TIMEOUT CASE4 TEST - server doesn't receive DATA and keeps sending ACK
                // if(cli_tftp->blockNum == 30)
                // {
                //     sleep(8);
                // }

                // send DATA 
                bzero(cli_tftp->outStream,SSIZE);   
                cli_tftp->snByte = cli_tftp->send_dta(cli_tftp,cli_tftp->fp,cli_tftp->outStream,cli_tftp->blockNum);
                alarm(2);
            }

            while(1)
            {
                // wait response
                bzero(cli_tftp->inStream,SSIZE); 
                cli_tftp->recByte = cli_tftp->get_resp(cli_tftp, cli_tftp->inStream);
                if(cli_tftp->recByte < 0)
                {
                    if(errno = EINTR)
                    {
                        if(cli_tftp->blockNum == 0)
                        {
                            cli_tftp->retxREQ(cli_tftp);                                                           // retransmit WRQ when failed to receive ACK #0
                        }
                        else
                        {
                            cli_tftp->retxDATA(cli_tftp);                                                          // retransmit DATA when failed to receive the corresponding ACK
                        }
                    }
                    else
                    {
                        exit(0);
                    }
                }
                else
                {
                    // extract opcode from the response
                    cli_tftp->opcode = cli_tftp->get_opc(cli_tftp, cli_tftp->inStream);
                    cli_tftp->timeout_reset();
                }

                // when the reponse is ACK
                if(cli_tftp->opcode == ACK)
                {
                    if(cli_tftp->blockNum > ntohs(*(short*)(cli_tftp->inStream+2)))                                 // discard duplicated ACK
                    {
                        alarm(2);
                        continue;
                    }
                    cli_tftp->blockNum = ntohs(*(short*)(cli_tftp->inStream+2));
                    printf("Received Ack #%d\n",cli_tftp->blockNum);
                    break;
                }
                // when the reponse is ERROR
                else if(cli_tftp->opcode == ERROR)
                {
                    short errCode = ntohs(*(short*)(cli_tftp->inStream+2));
                    char errMsg[50];
                    strcpy(errMsg,(cli_tftp->inStream+4));
                    printf("Received ERROR #%d: %s\n",errCode,errMsg);
                    fclose(cli_tftp->fp);
                    cli_tftp->fp = NULL;
                    exit(0);
                }
            }
            cli_tftp->blockNum++;
        } while (cli_tftp->snByte == 516);
    }

    if(cli_tftp->fp != NULL)
    {
        fclose(cli_tftp->fp);
        cli_tftp->fp = NULL;
    }

    printf("Finished!\n");
}

int main(int argc, char *argv[])
{
    cli_tftp = tftp_init(argv[0]);

    // set client port number, request type, and filename 
    if(argc == 5 && strcmp(argv[1],"-p") == 0)
    {
      cli_tftp->build_cli(cli_tftp, atoi(argv[2]));             // set client ip and port and bind them with a socket
      opc = argv[3];
      fileName = argv[4];
    }
    else
    {
      cli_tftp->build_cli(cli_tftp, -1);
      opc = argv[1];
      fileName = argv[2];
    }

    // register signal handler
    if(cli_tftp->register_handler(cli_tftp) != 0)
    {
        printf("failed to register timeout\n"); 
	}
    
    // start to run TFTP client
    run_client();

    // terminate client
    close(cli_tftp->sockfd);
    tftp_free(cli_tftp);

    return 0;
}
