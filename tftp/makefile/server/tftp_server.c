#include "../common/tftp.h"
#include <pthread.h>

pthread_t tid;
int retval;

void* process_client(void* my_tftp);

void run_server(struct tftp* serv_tftp)
{
    while(1)
    {
        // receive request
        bzero(serv_tftp->inStream,SSIZE);
        serv_tftp->get_resp(serv_tftp,serv_tftp->inStream);  

        // build a new socket and port
        struct tftp* new_tftp = tftp_init("./server");
        new_tftp->build_serv(new_tftp,0);
        // pass the client's ip and port
        new_tftp->cli_addr.sin_addr = serv_tftp->cli_addr.sin_addr;
        new_tftp->cli_addr.sin_port = serv_tftp->cli_addr.sin_port;
        // pass request to new socket
        bzero(new_tftp->inStream,SSIZE);
        memcpy(new_tftp->inStream,serv_tftp->inStream,SSIZE);

        // create a thread
        retval = pthread_create(&tid, NULL, process_client, new_tftp);
        if(retval != 0)
        {
            close(new_tftp->sockfd);
            tftp_free(new_tftp);
        }
    }
}

void* process_client(void* my_tftp) 
{   
    struct tftp* serv_tftp = my_tftp;
    serv_tftp->blockNum = 0;
    char client_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&serv_tftp->cli_addr.sin_addr,client_addr,sizeof(client_addr));
    int client_port = ntohs(serv_tftp->cli_addr.sin_port);

    // TIMEOUT CASE5,6 TEST - client doesn't receive ACK for the request and keeps sending RRQ or WRQ
    // sleep(8);

    // parse the received request; extract filename, mode
    serv_tftp->parse_req(serv_tftp->fileName,serv_tftp->mode,serv_tftp->inStream);
    // extract opcode from the request
    serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
    
    //check the filename to make sure it is not a filepath
    char * ptr;
    ptr = strchr(serv_tftp->fileName, '/');
    
      if(ptr != nullptr){
        bzero(serv_tftp->outStream,SSIZE);  
        serv_tftp->send_err(serv_tftp,serv_tftp->outStream,3);
        exit(0);
      }
      
    // process RRQ request
    if(serv_tftp->opcode == RRQ)
    {   
        printf("[UDP/%s:%d] Received[Read Request]\n",client_addr,client_port);
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
                exit(0);
            }
        }
        else                                                                                // server doesn't have the requested file 
        {
            bzero(serv_tftp->outStream,SSIZE);
            serv_tftp->send_err(serv_tftp,serv_tftp->outStream,0);
            exit(0);
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
            alarm(TIMEOUT);

            while(1)
            {
                // wait response
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
                    if(serv_tftp->opcode == RRQ)                                                                // discard duplicated RRQs 
                    {
                        alarm(TIMEOUT);
                        continue;
                    }
                    serv_tftp->timeout_reset(); 
                }
                // receive ACK 
                if(serv_tftp->opcode == ACK)
                {
                    if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))                              // discard duplicated ACKs 
                    {
                        alarm(TIMEOUT);
                        continue;
                    }
                    serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                    printf("[UDP/%s:%d] Received Ack #%d\n",client_addr,client_port,serv_tftp->blockNum);
                    break;
                }
            }
            serv_tftp->blockNum++;
        } while (serv_tftp->snByte == 516);                                                                        // terminate when all contents of the file has sended
    }
    // process WRQ request
    else if(serv_tftp->opcode == WRQ)
    {
        printf("[UDP/%s:%d] Received[Write Request]\n",client_addr,client_port);
        if(access(serv_tftp->fileName,F_OK) == 0)                                                           // check if the file exists
        {
            bzero(serv_tftp->outStream,SSIZE); 
            // send ERR
            serv_tftp->send_err(serv_tftp,serv_tftp->outStream,1);         
            exit(0);
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
                // wait response
                bzero(serv_tftp->inStream,SSIZE); 
                serv_tftp->recByte = serv_tftp->get_resp(serv_tftp, serv_tftp->inStream);
                if(serv_tftp->recByte < 0)
                {
                    if(errno = EINTR)
                    {
                        int dc = serv_tftp->retxACK(serv_tftp);                                             // retransmit ACK when failed to receive the corresponding DATA
                        if(dc == -1)                                                                        // client terminated
                        {
                            serv_tftp->recByte = 0;                                                         // set 0 to escape the outer while loop
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
                        alarm(TIMEOUT);
                        continue;
                    } 
                    serv_tftp->timeout_reset();
                }

                if(serv_tftp->opcode == DATA)
                {
                    if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))                            // discard duplicated DATA 
                    {
                        alarm(TIMEOUT);
                        continue;
                    }
                    serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                    printf("[UDP/%s:%d] Received Block #%d of data: %d byte(s)\n",client_addr,client_port,serv_tftp->blockNum,(serv_tftp->recByte-4));
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

    // terminate the thread
    close(serv_tftp->sockfd);
    tftp_free(serv_tftp);                                                                                             
}

int main(int argc, char *argv[])
{
    struct tftp* serv_tftp = tftp_init(argv[0]);

    // set port number
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      serv_tftp->build_serv(serv_tftp, atoi(argv[2]));         // set server ip and port and bind them with a socket
    }
    else
    {
      serv_tftp->build_serv(serv_tftp, -1);
    }

    // register signal handler
    if(serv_tftp->register_handler(serv_tftp)!=0)              
    {
        printf("failed to register timeout\n"); 
	}

    // start to run TFTP server
    run_server(serv_tftp);                                     

    close(serv_tftp->sockfd);
    tftp_free(serv_tftp);

    return 0;
}
