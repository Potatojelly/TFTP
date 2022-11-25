#include "../common/tftp.h"

struct tftp* serv_tftp;

void run_server() 
{   
    while(1) 
    {
        serv_tftp->blockNum = 0;
        printf("Receiving Request...\n");

        // TIMEOUT CASE5,6 TEST - client doesn't receive ACK for the request and keeps sending RRQ or WRQ
        // sleep(8);

        // receive request 
        bzero(serv_tftp->inStream,SSIZE);
        serv_tftp->get_resp(serv_tftp,serv_tftp->inStream);  

        /* 
            parse the received request
            extract filename, mode
        */
        serv_tftp->parse_req(serv_tftp->fileName,serv_tftp->mode,serv_tftp->inStream);
        // extract opcode from the request
        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
        
        // request: RRQ
        if(serv_tftp->opcode == RRQ)
        {   
            printf("Received[Read Request]\n");
            // open file
            if(access(serv_tftp->fileName,F_OK) == 0)                                           // check if the file exists
            {
                if(access(serv_tftp->fileName,R_OK) == 0)                                       // check if the file can be read
                {
                    serv_tftp->fp = fopen(serv_tftp->fileName,"r");
                    serv_tftp->blockNum++;                                                      // set blockNum = 1 to send DATA #1
                }
                else                                                                            // server has no permission to read the file
                {
                    bzero(serv_tftp->outStream,SSIZE);  
                    serv_tftp->send_err(serv_tftp,serv_tftp->outStream,2);
                    continue;
                }
            }
            else                                                                                // server doesn't have the requested file 
            {
                bzero(serv_tftp->outStream,SSIZE);
                serv_tftp->send_err(serv_tftp,serv_tftp->outStream,0);
                continue;
            }

            do
            {
                // TIMEOUT CASE1 TEST - client doesn't recevie DATA and keeps sending ACK
                // if(serv_tftp->blockNum == 30)
                // {
                //     sleep(8);
                // }

                // send DATA 
                bzero(serv_tftp->outStream,SSIZE); 
                serv_tftp->snByte = serv_tftp->send_dta(serv_tftp,serv_tftp->fp,serv_tftp->outStream,serv_tftp->blockNum); 
                alarm(2);

                while(1)
                {
                    bzero(serv_tftp->inStream,SSIZE);
                    serv_tftp->recByte = serv_tftp->get_resp(serv_tftp,serv_tftp->inStream);
                    if(serv_tftp->recByte < 0)
                    {
                        if(errno = EINTR)
                        {
                            int dc = serv_tftp->retxDATA(serv_tftp);                                         // retransmit DATA when failed to receive the corresponding ACK
                            if(dc == -1)                                                                     // client terminated
                            {
                                serv_tftp->snByte = 0;                                                       // escape the outer while loop
                                break; 
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
                        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
                        if(serv_tftp->opcode == RRQ)                                                // discard duplicated RRQs 
                        {
                            alarm(2);
                            continue;
                        }
                        serv_tftp->timeout_reset(); 
                    }
                    // receive ACK 
                    if(serv_tftp->opcode == ACK)
                    {
                        if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))            // discard duplicated ACKs 
                        {
                            alarm(2);
                            continue;
                        }
                        serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                        printf("Received Ack #%d\n",serv_tftp->blockNum);
                        break;
                    }
                }
                serv_tftp->blockNum++;
            } while (serv_tftp->snByte == 516);                                                     // terminate when all contents of the file has sended
        }
        // request: WRQ
        else if(serv_tftp->opcode == WRQ)
        {
            printf("Received[Write Request]\n");
            if(access(serv_tftp->fileName,F_OK) == 0)                                               // check if the file exists
            {
                bzero(serv_tftp->outStream,SSIZE); 
                // send ERR
                serv_tftp->send_err(serv_tftp,serv_tftp->outStream,1);         
                continue;
            }
            else
            {
                serv_tftp->fp = fopen(serv_tftp->fileName,"w");
                // send ACK for the WRQ request
                bzero(serv_tftp->outStream,SSIZE); 
                serv_tftp->send_ack(serv_tftp, serv_tftp->outStream, serv_tftp->blockNum);    
                alarm(2);
            }

            do
            {
                while(1)
                {
                    // receive response
                    bzero(serv_tftp->inStream,SSIZE); 
                    serv_tftp->recByte = serv_tftp->get_resp(serv_tftp, serv_tftp->inStream);
                    if(serv_tftp->recByte < 0)
                    {
                        if(errno = EINTR)
                        {
                            int dc = serv_tftp->retxACK(serv_tftp);                                             // retransmit ACK when failed to receive the corresponding DATA
                            if(dc == -1)                                                                        // client terminated
                            {
                                serv_tftp->recByte = 0;                                                          // escape the outer while loop
                                break; 
                            }
                        }
                        else
                        {
                            exit(0);
                        }
                    }
                    else
                    {
                        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
                        if(serv_tftp->opcode == WRQ)                                                               // discard duplicated WRQ
                        {
                            alarm(2);
                            continue;
                        } 
                        serv_tftp->timeout_reset();
                    }

                    if(serv_tftp->opcode == DATA)
                    {
                        if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))                            // discard duplicated DATA 
                        {
                            alarm(2);
                            continue;
                        }
                        serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                        printf("Received Block #%d of data: %d byte(s)\n",serv_tftp->blockNum,(serv_tftp->recByte-4));
                        fwrite(serv_tftp->inStream+4,1,serv_tftp->recByte-4,serv_tftp->fp); 
                    }

                    // TIMEOUT CASE3 TEST  - client doesn't recevie ACK and keeps sending DATA
                    // if(serv_tftp->blockNum == 30)
                    // {
                    //     sleep(8);
                    // }
                    
                    // send ACK
                    bzero(serv_tftp->outStream,SSIZE); 
                    serv_tftp->send_ack(serv_tftp, serv_tftp->outStream, serv_tftp->blockNum);    
                    if(serv_tftp->recByte == 516)                                                                       // don't set alarm after sending ACK when all packets are received
                    {
                        alarm(2);
                    }
                    break;
                }
               serv_tftp->blockNum++;
            } while (serv_tftp->recByte == 516);
        }  

        if(serv_tftp->fp != NULL)
        {
            fclose(serv_tftp->fp);
            serv_tftp->fp = NULL;
        }
    }
}

int main(int argc, char *argv[])
{
    serv_tftp = tftp_init(argv[0]);
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      serv_tftp->build_serv(serv_tftp, atoi(argv[2]));
    }
    else
    {
      serv_tftp->build_serv(serv_tftp, -1);
    }

    if(serv_tftp->register_handler(serv_tftp)!=0)
    {
        printf("failed to register timeout\n"); 
	}

    run_server();

    close(serv_tftp->sockfd);
    free(serv_tftp);

    return 0;
}
