#include "../common/tftp.h"
#include <pthread.h>

pthread_t tid;
int retval;
sigset_t sigSetMask;
bool check = false; // for TIMEOUT TEST

void* process_client(void* my_tftp);

void run_server(struct tftp* serv_tftp)
{
    // block SIGALRM sent to the main thread
    sigaddset(&sigSetMask, SIGALRM);
    sigprocmask(SIG_BLOCK,&sigSetMask,NULL);
    int oldCliPortNum = -1;
    int currentCliPortNum = -1;

    while(1)
    {
        // TIMEOUT CASE5,6 TEST - client doesn't receive ACK for the request and keeps sending RRQ or WRQ
        // if(!check)
        // {
        //     check = true; // test once
        //     sleep(8);
        // }

        // receive request
        bzero(serv_tftp->inStream,SSIZE);
        serv_tftp->get_resp(serv_tftp,serv_tftp->inStream);  
        // extract opcode from the request
        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
        // check clientPortNum to discard duplicated request;
        oldCliPortNum = currentCliPortNum; 
        currentCliPortNum = ntohs(serv_tftp->cli_addr.sin_port);

        if((oldCliPortNum != currentCliPortNum) && (serv_tftp->opcode == RRQ || serv_tftp->opcode == WRQ))
        {
            // build a new socket and port
            struct tftp* new_tftp = tftp_init("./server");
            new_tftp->build_servThread(new_tftp);
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
}

void* process_client(void* my_tftp) 
{   
    struct tftp* serv_tftp = my_tftp;
    serv_tftp->blockNum = 0;
    char client_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&serv_tftp->cli_addr.sin_addr,client_addr,sizeof(client_addr));
    int client_port = ntohs(serv_tftp->cli_addr.sin_port);
    // unblock SIGALRM sent to process client threads
    pthread_sigmask(SIG_UNBLOCK,&sigSetMask,NULL);

    // parse the received request; extract filename, mode
    serv_tftp->parse_req(serv_tftp->fileName,serv_tftp->mode,serv_tftp->inStream);
    // check if mode is octet
    if(strcmp(serv_tftp->mode,MODE) != 0) 
    {
        bzero(serv_tftp->outStream,SSIZE);
        serv_tftp->send_err(serv_tftp,serv_tftp->outStream,4);
        pthread_exit(PTHREAD_CANCELED);
    }
    //check the filename to make sure it is not a filepath
    char * ptr;
    ptr = strchr(serv_tftp->fileName, '/');

    if(ptr != NULL)
    {
        bzero(serv_tftp->outStream,SSIZE);  
        serv_tftp->send_err(serv_tftp,serv_tftp->outStream,3);
        pthread_exit(PTHREAD_CANCELED);
    }
    // extract opcode from the request
    serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
    
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
                // send DATA 
                bzero(serv_tftp->outStream,SSIZE); 
                serv_tftp->snByte = serv_tftp->send_dta(serv_tftp,serv_tftp->fp,serv_tftp->outStream,serv_tftp->blockNum); 
                alarm(TIMEOUT);
            }
            else                                                                            // server has no permission to read the file
            {
                // send ERROR
                bzero(serv_tftp->outStream,SSIZE);  
                serv_tftp->send_err(serv_tftp,serv_tftp->outStream,2);
                pthread_exit(PTHREAD_CANCELED);
            }
        }
        else                                                                                // server doesn't have the requested file 
        {
            // send ERROR
            bzero(serv_tftp->outStream,SSIZE);
            serv_tftp->send_err(serv_tftp,serv_tftp->outStream,0);
            pthread_exit(PTHREAD_CANCELED);
        }
    }
    // process WRQ request
    else if(serv_tftp->opcode == WRQ)
    {
        printf("[UDP/%s:%d] Received[Write Request]\n",client_addr,client_port);
        if(access(serv_tftp->fileName,F_OK) == 0)                                                           // check if the file exists
        {
            bzero(serv_tftp->outStream,SSIZE); 
            // send ERROR
            serv_tftp->send_err(serv_tftp,serv_tftp->outStream,1);         
            pthread_exit(PTHREAD_CANCELED);
        }
        else
        {
            serv_tftp->fp = fopen(serv_tftp->fileName,"w");
            // send ACK for the WRQ request
            bzero(serv_tftp->outStream,SSIZE); 
            serv_tftp->send_ack(serv_tftp, serv_tftp->outStream, serv_tftp->blockNum);    
            alarm(TIMEOUT);
        }
    }

    while(1)
    {
        // wait response
        bzero(serv_tftp->inStream,SSIZE);
        serv_tftp->recByte = serv_tftp->get_resp(serv_tftp, serv_tftp->inStream);
        // data loss control
        if(serv_tftp->recByte < 0)
        {
            if(errno == EINTR)
            {
                int dc = 0;
                if(serv_tftp->opcode == DATA)
                {
                    dc = serv_tftp->retxDATA(serv_tftp);  
                }
                else if(serv_tftp->opcode == ACK)
                {
                    dc = serv_tftp->retxACK(serv_tftp);
                }
                if(dc == -1)                                                                     // client terminated
                {
                    break; 
                }
                continue;
            }
            else
            {
                pthread_exit(PTHREAD_CANCELED);
            }
        }
        else
        {
            serv_tftp->timeout_reset();
        }

        // extract opcode from the response 
        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);

            // check opcode of response
        if(serv_tftp->opcode == DATA)
        {
            // next DATA corresponding sent ACK
            if((serv_tftp->blockNum+1) == ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                printf("[UDP/%s:%d] Received Block #%d of data: %d byte(s)\n",client_addr,client_port,serv_tftp->blockNum,(serv_tftp->recByte-4));
                fwrite(serv_tftp->inStream+4,1,serv_tftp->recByte-4,serv_tftp->fp);                                 // make a file with received DATA 
            }
            // discard duplicated DATA: send ACK
            else if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                printf("[UDP/%s:%d] Received Block #%d of data: %d byte(s)\n",client_addr,client_port,serv_tftp->blockNum,(serv_tftp->recByte-4));
                continue;
            }
            // send ACK
            // TIMEOUT CASE3 TEST  - client doesn't recevie ACK and keeps sending DATA
            // if(serv_tftp->blockNum == 5 && !check)
            // {
            //     check = true; // to prevent to sleep as much as times that it receives duplicated DATA
            //     sleep(8);
            // }
            bzero(serv_tftp->outStream,SSIZE); 
            serv_tftp->snByte = serv_tftp->send_ack(serv_tftp, serv_tftp->outStream, serv_tftp->blockNum);
            if(serv_tftp->recByte == 516)
            {
                alarm(TIMEOUT);
            }
            // last DATA to receive
            else
            {
                break;
            }
        }
        else if (serv_tftp->opcode == ACK)
        {
            // ACK to corresponding sent DATA
            if(serv_tftp->blockNum == ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                printf("[UDP/%s:%d] Received Ack #%d\n",client_addr,client_port,serv_tftp->blockNum);
            }
            // discard duplicated ACK
            else if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                alarm(TIMEOUT);
                continue;
            }
            // send next DATA
            // TIMEOUT CASE1 TEST - client doesn't recevie DATA and keeps sending ACK
            // if(serv_tftp->blockNum == 5)
            // {
            //     sleep(8);
            // }
            serv_tftp->blockNum++;
            bzero(serv_tftp->outStream,SSIZE);   
            serv_tftp->snByte = serv_tftp->send_dta(serv_tftp,serv_tftp->fp,serv_tftp->outStream,serv_tftp->blockNum);
            if(serv_tftp->snByte == 516)
            {
                alarm(TIMEOUT);
            }
            // last DATA to send
            else
            {
                alarm(TIMEOUT);
                // wait response
                bzero(serv_tftp->inStream,SSIZE);
                serv_tftp->recByte = serv_tftp->get_resp(serv_tftp, serv_tftp->inStream);
                // extract opcode from the request
                serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
                // ACK to corresponding sent DATA
                if(serv_tftp->blockNum == ntohs(*(short*)(serv_tftp->inStream+2)))
                {
                    printf("[UDP/%s:%d] Received Ack #%d\n",client_addr,client_port,serv_tftp->blockNum);
                }
                // discard duplicated ACK
                else if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))
                {
                    alarm(TIMEOUT);
                }
                break;
            }
        }
        else if (serv_tftp->opcode == ERROR)
        {
            char errMsg[50];
            short errCode = ntohs(*(short*)(serv_tftp->inStream+2));
            strcpy(errMsg,(serv_tftp->inStream+4));
            printf("Received ERROR #%d: %s\n",errCode,errMsg);
            if(serv_tftp->fp != NULL)
            {
                fclose(serv_tftp->fp);
                serv_tftp->fp = NULL;
            }
            if(errCode == 0 || errCode == 2)                                                                 // not found or no permission
            {
                remove(serv_tftp->fileName);                                                                  // remove the file created to read from server
            }
            exit(0);
        }
    }

    if(serv_tftp->fp != NULL)
    {
        fclose(serv_tftp->fp);
        serv_tftp->fp = NULL;
    }

    // terminate the thread
    close(serv_tftp->sockfd);
    tftp_free(serv_tftp);    
    printf("Finished!\n");
}

    //##########################################################################

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