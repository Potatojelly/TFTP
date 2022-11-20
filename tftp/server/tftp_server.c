#include "../common/tftp.h"

struct tftp* serv_tftp;
int count;
int recByte = 0; 
int snByte = 0;
int opc;
char opcode[3];
char fileName[20];
char mode[10];
char outStream[SSIZE];
char inStream[SSIZE];
short blockNum = 0;
FILE * fp = NULL;
//void (*old_hanlder)() = 

// alarm hanlder
void retxDATA(int signal)
{
    count++;
    if(count <= 10)
    {
        snByte = serv_tftp->send_dta(serv_tftp,fp,outStream,blockNum); 
        alarm(2); 
    }
    else
    {
        printf("server: 10 consecutive timesouts has occurd.\n");
        count = 0;
        alarm(0);
        fclose(fp);
    }
}

void run_server() 
{   
    while(1) 
    {
        blockNum = 0;
        // receive RQ
        serv_tftp->get_resp(serv_tftp,inStream);
        // parse RQ
        serv_tftp->parse_req(fileName,mode,inStream);
        // get opcode
        opc = serv_tftp->get_opc(serv_tftp, inStream);
        
        if(opc == RRQ)
        {   
            printf("Received[Read Request]\n");
            // open file
            if(access(fileName,F_OK) == 0)                                                      // check if the file exists
            {
                if(access(fileName,R_OK) == 0)
                {
                    fp = fopen(fileName,"r");
                }
                else                                                                            // server has no permission to read the file
                {
                    bzero(outStream,SSIZE); 
                    serv_tftp->send_err(serv_tftp,outStream,2);
                    continue;
                }
            }
            else                                                                                 // server doesn't have the requested file from client 
            {
                bzero(outStream,SSIZE);
                serv_tftp->send_err(serv_tftp,outStream,0);
                continue;
            }

            do
            {
                // send DATA 
                bzero(outStream,SSIZE); 
                snByte = serv_tftp->send_dta(serv_tftp,fp,outStream,blockNum); 

                bzero(inStream,SSIZE);
                serv_tftp->get_resp(serv_tftp,inStream);
                // get opcode
                opc = serv_tftp->get_opc(serv_tftp, inStream);
                // receive ACK 
                if(opc == ACK)
                {
                    blockNum = ntohs(*(short*)(inStream+2));
                    printf("Received Ack #%d\n",blockNum);
                }
                blockNum++;
            } while (snByte == 516);
        }
        else if(opc == WRQ)
        {
            printf("Received[Write Request]\n");
            if(access(fileName,F_OK) == 0)                                                         // check if the file exists
            {
                bzero(outStream,SSIZE); 
                // send ERR
                serv_tftp->send_err(serv_tftp,outStream,1);         
                continue;
            }
            else
            {
                fp = fopen(fileName,"w");
                // send ACK
                bzero(outStream,SSIZE); 
                serv_tftp->send_ack(serv_tftp, outStream, blockNum);    
            }

            do
            {
                // receive response
                bzero(inStream,SSIZE); 
                recByte = serv_tftp->get_resp(serv_tftp, inStream);
                // get opcode
                opc = serv_tftp->get_opc(serv_tftp, inStream);
                if(opc == DATA)
                {
                    blockNum = ntohs(*(short*)(inStream+2));
                    printf("Received Block #%d of data: %d byte(s)\n",blockNum,recByte);
                    fwrite(inStream+4,1,recByte-4,fp); 
                }
                blockNum++;
                // send ACK
                bzero(outStream,SSIZE); 
                serv_tftp->send_ack(serv_tftp, outStream, blockNum);    
            } while (recByte == 516);
        }  
        fclose(fp);
    }

}


int main(int argc, char *argv[])
{
    // signal(SIGALRM, retxDATA); // set a signal hanlder
    serv_tftp = tftp_init(argv[0]);
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      serv_tftp->build_serv(serv_tftp, atoi(argv[2]));
    }
    else
    {
      serv_tftp->build_serv(serv_tftp, -1);
    }

    run_server();

    close(serv_tftp->sockfd);
    free(serv_tftp);

    return 0;
}
