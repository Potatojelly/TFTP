#include "../common/tftp.h"

struct tftp* cli_tftp;
char* opc;
char* fileName;
// bool check = false; 


/**
 * void run_client()
 * requests a RRQ or WRQ request, processes or sends DATA, ACK, and ERROR from or to server   
 * 
 * @ param struct tftp* 
 * @ return void
 */
void run_client()
{
     cli_tftp->blockNum = 0;

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

     // request RRQ
    if(cli_tftp->opcode == RRQ)   
    {
        // check if the file already exists
        if(access(cli_tftp->fileName,F_OK) == 0)                                                                
        {
            printf("client: file is already exist error(overwrite warning)\n");
            exit(0);
        }
        else
        {
            cli_tftp->fp = fopen(cli_tftp->fileName,"w");
            // send RRQ 
            bzero(cli_tftp->outStream,SSIZE);                                                                    
            cli_tftp->snByte = cli_tftp->send_req(cli_tftp, cli_tftp->opcode, cli_tftp->fileName, MODE, cli_tftp->outStream);
            alarm(TIMEOUT);
        }
    }
    // request WRQ
    else if(cli_tftp->opcode == WRQ)   
    {
        if(access(cli_tftp->fileName,F_OK) == 0)                                                                
        {
            if(access(cli_tftp->fileName,R_OK) == 0)                                                             
            {
                cli_tftp->fp = fopen(cli_tftp->fileName,"r");
                // send WRQ 
                bzero(cli_tftp->outStream,SSIZE); 
                cli_tftp->send_req(cli_tftp, cli_tftp->opcode, cli_tftp->fileName, MODE, cli_tftp->outStream);
                cli_tftp->snByte = 516; // to prevent stop process
                alarm(TIMEOUT);
            }
            // no permission to read the file
            else                                                                                                 
            {
                printf("client: no permission to open the file error\n");
                exit(0);
            }
        }
        // doesn't have the file to write to server
        else                                                                                                     
        {
            printf("client: file not found error\n");
            exit(0);
        }
    }

    while(1)
    {
        // wait response
        bzero(cli_tftp->inStream,SSIZE);
        cli_tftp->recByte = cli_tftp->get_resp(cli_tftp, cli_tftp->inStream);
        // packet loss control
        if(cli_tftp->recByte < 0)
        {
            if(errno == EINTR)
            {
                if(cli_tftp->opcode == RRQ)
                {
                    cli_tftp->retxREQ(cli_tftp);   
                }
                else if(cli_tftp->opcode == WRQ)
                {
                    cli_tftp->retxREQ(cli_tftp);  
                }
                else if(cli_tftp->opcode == DATA)
                {
                    cli_tftp->retxDATA(cli_tftp);  
                }
                else if(cli_tftp->opcode == ACK)
                {
                    cli_tftp->retxACK(cli_tftp);
                }
                continue;
            }
            else
            {
                exit(0);
            }
        }
        else
        {
            cli_tftp->timeout_reset();
        }

        cli_tftp->opcode = cli_tftp->get_opc(cli_tftp, cli_tftp->inStream);

        if(cli_tftp->opcode == DATA)
        {
            // DATA corresponding to ACK sent previous
            if((cli_tftp->blockNum+1) == ntohs(*(short*)(cli_tftp->inStream+2)))
            {
                cli_tftp->blockNum = ntohs(*(short*)(cli_tftp->inStream+2));
                printf("Received Block #%d of data: %d byte(s)\n",cli_tftp->blockNum,(cli_tftp->recByte-4));
                fwrite(cli_tftp->inStream+4,1,cli_tftp->recByte-4,cli_tftp->fp);                                 
            }
            // hanlde duplicated DATA; sends ACK
            else if(cli_tftp->blockNum > ntohs(*(short*)(cli_tftp->inStream+2)))
            {
                printf("Received Block #%d of data: %d byte(s)\n",cli_tftp->blockNum,(cli_tftp->recByte-4));
                continue;
            }

            // TIMEOUT CASE3 TEST  - server doesn't receive ACK and keeps sending DATA
            // if(cli_tftp->blockNum == 5 && !check)
            // {
            //     check = true; // to prevent to sleep as much as times that it receives duplicated DATA
            //     sleep(8);
            // }
            bzero(cli_tftp->outStream,SSIZE); 
            cli_tftp->snByte = cli_tftp->send_ack(cli_tftp, cli_tftp->outStream, cli_tftp->blockNum);
            if(cli_tftp->recByte == 516)
            {
                alarm(TIMEOUT);
            }
            // last DATA to receive
            else
            {
                break;
            }
        }
        else if(cli_tftp->opcode == ACK)
        {
            // ACK corresponding to DATA sent previous
            if(cli_tftp->blockNum == ntohs(*(short*)(cli_tftp->inStream+2)))
            {
                printf("Received Ack #%d\n",cli_tftp->blockNum);
            }
            // discard duplicated ACK
            else if(cli_tftp->blockNum > ntohs(*(short*)(cli_tftp->inStream+2)))
            {
                alarm(TIMEOUT);
                continue;
            }

            // TIMEOUT CASE4 TEST - server doesn't receive DATA and keeps sending ACK
            // if(cli_tftp->blockNum == 5)
            // {
            //     sleep(8);
            // }

            if(cli_tftp->snByte < 516)
            {
                break;
            }

            cli_tftp->blockNum++;
            bzero(cli_tftp->outStream,SSIZE);   
            cli_tftp->snByte = cli_tftp->send_dta(cli_tftp,cli_tftp->fp,cli_tftp->outStream,cli_tftp->blockNum);
            alarm(TIMEOUT);
        }
        else if(cli_tftp->opcode == ERROR)
        {
            char errMsg[50];
            short errCode = ntohs(*(short*)(cli_tftp->inStream+2));
            strcpy(errMsg,(cli_tftp->inStream+4));
            printf("Received ERROR #%d: %s\n",errCode,errMsg);
            if(cli_tftp->fp != NULL)
            {
                fclose(cli_tftp->fp);
                cli_tftp->fp = NULL;
            }
            // server hasn't found or has no permission to the requested file 
            if(errCode == 0 || errCode == 2)                                                                 
            {
                 // remove the file created to read from server
                remove(cli_tftp->fileName);                                                                 
            }
            exit(0);
        }
    }

    if(cli_tftp->fp != NULL)
    {
        fclose(cli_tftp->fp);
        cli_tftp->fp = NULL;
    }
    printf("Complete!\n");
}

/**
 * int main(int, char*)
 * recevies port#, request type at runtime, builds UDP client socket, and starts run TFTP client
 * 
 * @ param int, char*
 * @ return 0
 * 
*/
int main(int argc, char *argv[])
{
    cli_tftp = tftp_init(argv[0]);

    // set client port number, request type, and filename 
    if(argc == 3 && (strcmp(argv[1],"-r") == 0 || strcmp(argv[1],"-w") == 0) )
    {
      // set client ip and port and bind them with a socket  
      cli_tftp->build_cli(cli_tftp, -1); // builds with a default port#
      opc = argv[1];
      fileName = argv[2];
    }
    else if(argc == 5 && strcmp(argv[1],"-p") == 0 && ((strcmp(argv[1],"-r") == 0 || strcmp(argv[1],"-w") == 0)))
    {
      cli_tftp->build_cli(cli_tftp, atoi(argv[2]));             
      opc = argv[3];
      fileName = argv[4];
    }
    else
    {
        printf("client:invalid command error\n");
        exit(0);
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
