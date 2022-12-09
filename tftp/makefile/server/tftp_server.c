#include "../common/tftp.h"
#include <pthread.h>

pthread_t tid;
int retval;          
sigset_t sigSetMask; // struct to block or unblock signal 
// bool check = false;  // variable to make sleep() occur once when testing timeout case 

void* process_client(void* my_tftp);

/**
 * void run_server(struct tftp*)
 * recevies a RRQ or WRQ request from a client 
 * and passes it to a sub thread created with new socket and new port# to process the request 
 * 
 * @ param struct tftp* 
 * @ return void
 * 
*/
void run_server(struct tftp* serv_tftp)
{
    sigaddset(&sigSetMask, SIGALRM);
    sigprocmask(SIG_BLOCK,&sigSetMask,NULL); // block SIGALRM 

    int oldCliPortNum = -1;
    int currentCliPortNum = -1;

    while(1)
    {
        // TIMEOUT CASE5,6 TEST - client doesn't receive ACK for the request and keeps sending RRQ or WRQ
        if(!check)
        {
            check = true; // test once
            sleep(8);
        }

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

            // create a sub thread
            retval = pthread_create(&tid, NULL, process_client, new_tftp);
            if(retval != 0)
            {
                close(new_tftp->sockfd);
                tftp_free(new_tftp);
            }
        }
    }
}


/**
 * void* process_client(void*)
 * recevies a RRQ or WRQ request from the main thread and process the request  
 * 
 * @ param void* 
 * @ return void
 * 
*/
void* process_client(void* my_tftp) 
{   
    struct tftp* serv_tftp = my_tftp;
    serv_tftp->blockNum = 0;

    // variables to print out client's ip address and port#
    char client_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&serv_tftp->cli_addr.sin_addr,client_addr,sizeof(client_addr));
    int client_port = ntohs(serv_tftp->cli_addr.sin_port);

    // unblock SIGALRM 
    pthread_sigmask(SIG_UNBLOCK,&sigSetMask,NULL);

    // extract filename, mode from request 
    serv_tftp->parse_req(serv_tftp->fileName,serv_tftp->mode,serv_tftp->inStream);

    // check if mode is not octet
    if(strcmp(serv_tftp->mode,MODE) != 0) 
    {
        bzero(serv_tftp->outStream,SSIZE);
        serv_tftp->send_err(serv_tftp,serv_tftp->outStream,4);
        pthread_exit(PTHREAD_CANCELED);
    }

    //check if the filename is a filepath
    char * ptr;
    ptr = strchr(serv_tftp->fileName, '/');

    if(ptr != NULL)
    {
        bzero(serv_tftp->outStream,SSIZE);  
        serv_tftp->send_err(serv_tftp,serv_tftp->outStream,3);
        pthread_exit(PTHREAD_CANCELED);
    }

    serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);
    
    // process RRQ request
    if(serv_tftp->opcode == RRQ)
    {   
        printf("[UDP/%s:%d] Received[Read Request]\n",client_addr,client_port);

         // check if the file exists
        if(access(serv_tftp->fileName,F_OK) == 0)                                           
        {
            // check if the file can be read
            if(access(serv_tftp->fileName,R_OK) == 0)                                      
            {
                serv_tftp->fp = fopen(serv_tftp->fileName,"r");
                serv_tftp->blockNum++;  // set blockNum = 1 to send DATA #1
                // send DATA 
                bzero(serv_tftp->outStream,SSIZE); 
                serv_tftp->snByte = serv_tftp->send_dta(serv_tftp,serv_tftp->fp,serv_tftp->outStream,serv_tftp->blockNum); 
                alarm(TIMEOUT);
            }
            // no permission to read the file
            else                                                                            
            {
                // send ERROR
                bzero(serv_tftp->outStream,SSIZE);  
                serv_tftp->send_err(serv_tftp,serv_tftp->outStream,2);
                pthread_exit(PTHREAD_CANCELED);
            }
        }
        // doesn't have the requested file 
        else                                                                                
        {
            bzero(serv_tftp->outStream,SSIZE);
            serv_tftp->send_err(serv_tftp,serv_tftp->outStream,0);
            pthread_exit(PTHREAD_CANCELED);
        }
    }
    // process WRQ request
    else if(serv_tftp->opcode == WRQ)
    {
        printf("[UDP/%s:%d] Received[Write Request]\n",client_addr,client_port);
        if(access(serv_tftp->fileName,F_OK) == 0)                                                         
        {
            bzero(serv_tftp->outStream,SSIZE); 
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
        // packet loss control
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
                if(dc == -1)                                                                    
                {
                    break;  // client terminated, terminates the process
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

        serv_tftp->opcode = serv_tftp->get_opc(serv_tftp, serv_tftp->inStream);

        // opcode == DATA
        if(serv_tftp->opcode == DATA)
        {
            // DATA corresponding to ACK sent previous; eg) AKC#5 -> DATA#6
            if((serv_tftp->blockNum+1) == ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                serv_tftp->blockNum = ntohs(*(short*)(serv_tftp->inStream+2));
                printf("[UDP/%s:%d] Received Block #%d of data: %d byte(s)\n",client_addr,client_port,serv_tftp->blockNum,(serv_tftp->recByte-4));
                 // write a file with received DATA 
                fwrite(serv_tftp->inStream+4,1,serv_tftp->recByte-4,serv_tftp->fp);                                
            }
            // handle duplicated DATA; send ACK
            else if(serv_tftp->blockNum > ntohs(*(short*)(serv_tftp->inStream+2)))
            {
                printf("[UDP/%s:%d] Received Block #%d of data: %d byte(s)\n",client_addr,client_port,serv_tftp->blockNum,(serv_tftp->recByte-4));
                continue;
            }
        
            // TIMEOUT CASE1 TEST  - client doesn't recevie ACK and keeps sending DATA
            // if(serv_tftp->blockNum == 5 && !check)
            // {
            //     check = true; // prevents to sleep as much as times that it receives duplicated DATA
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
         // opcode == ACK
        else if (serv_tftp->opcode == ACK)
        {
            // ACK corresponding DATA sent previous
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

            // TIMEOUT CASE2 TEST - client doesn't recevie DATA and keeps sending ACK
            // if(serv_tftp->blockNum == 5)
            // {
            //     sleep(8);
            // }

            // last DATA to send 
            if(serv_tftp->snByte < 516)
            {
                break;
            }
            serv_tftp->blockNum++;
            bzero(serv_tftp->outStream,SSIZE);   
            serv_tftp->snByte = serv_tftp->send_dta(serv_tftp,serv_tftp->fp,serv_tftp->outStream,serv_tftp->blockNum);
            alarm(TIMEOUT);
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
    printf("Complete!\n");
}

/**
 * int main(int, char*)
 * can recevie port# at runtime, builds UDP server socket, and starts run TFTP server
 * 
 * @ param int, char*
 * @ return 0
 * 
*/
int main(int argc, char *argv[])
{
    struct tftp* serv_tftp = tftp_init(argv[0]);

    // set port number
    if(argc == 3 && strcmp(argv[1],"-p") == 0)
    {
      // set server ip and port and bind them with a socket
        serv_tftp->build_serv(serv_tftp, atoi(argv[2]));         
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