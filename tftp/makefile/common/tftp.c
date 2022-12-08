#include "tftp.h"

// initialize function pointers and variables of tftp
struct tftp* tftp_init(char* name) 
{
	struct tftp *my_tftp = malloc(sizeof(struct tftp));
  my_tftp->build_cli = build_cliSocket;
  my_tftp->build_serv = build_servSocket;
  my_tftp->build_servThread = build_servThreadSocket;
  my_tftp->parse_req = parse_request;
  my_tftp->send_req = send_request;
  my_tftp->retxREQ = retxREQ;
  my_tftp->send_dta = send_data;
  my_tftp->resend_dta = resend_data;
  my_tftp->retxDATA = retxDATA;
  my_tftp->send_ack = send_ack;
  my_tftp->retxACK = retxACK;
  my_tftp->send_err = send_error;
  my_tftp->get_resp = get_response;
  my_tftp->get_opc = get_opcode;
  my_tftp->handle_timeout = handle_timeout;
  my_tftp->register_handler = register_handler;
  my_tftp->timeout_reset = timeout_reset;

  my_tftp->sockfd = 0;
  my_tftp->clilen = sizeof(my_tftp->cli_addr);
  my_tftp->servlen = sizeof(my_tftp->serv_addr);
  my_tftp->progname = name;
  my_tftp->type = 0;
  my_tftp->recByte = 0; 
  my_tftp->snByte = 0;
  my_tftp->blockNum = 0;
  my_tftp->opcode = 0;

  memset((void*)&my_tftp->cli_addr,0,sizeof(my_tftp->cli_addr));
  memset((void*)&my_tftp->serv_addr,0,sizeof(my_tftp->serv_addr));

  memset((void*)&my_tftp->inStream,0,sizeof(my_tftp->inStream));
  memset((void*)&my_tftp->outStream,0,sizeof(my_tftp->outStream));
  my_tftp->fp = NULL;
  memset((void*)&my_tftp->fileName,0,sizeof(my_tftp->fileName));
  memset((void*)&my_tftp->mode,0,sizeof(my_tftp->mode));

	return my_tftp;
}

// deallocate tftp 
void tftp_free(struct tftp *my_tftp)
{
  if(my_tftp->fp != NULL)
  {
      fclose(my_tftp->fp);
      my_tftp->fp = NULL;
  }
  free(my_tftp);
}

// build server socket
void build_servSocket(struct tftp *my_tftp, int port)
{

  if(port < 0 || port > 65535){
    port = SERV_UDP_PORT;
  }
  
  my_tftp->type = SERVER;
  if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
  {
    printf("%s: can't open datagram socket\n",*(my_tftp->progname));
    exit(0);
  }
  // set server address
  bzero((char *) &(my_tftp->serv_addr), sizeof(my_tftp->serv_addr));
  my_tftp->serv_addr.sin_family = AF_INET;
  my_tftp->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_tftp->serv_addr.sin_port = htons(port);
      
  if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->serv_addr), my_tftp->servlen) < 0)
  {
    printf("%s: can't bind local address\n",my_tftp->progname);
    exit(0);
  }
  else
  {
    printf("%s: PORT NUMBER:%d\n",my_tftp->progname,ntohs(my_tftp->serv_addr.sin_port)); // check server port num
  }
}

// build server thread socket
void build_servThreadSocket(struct tftp *my_tftp)
{
  my_tftp->type = SERVER;
  if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
  {
    printf("%s: can't open datagram socket\n",*(my_tftp->progname));
    exit(0);
  }
  // set server address
  bzero((char *) &(my_tftp->serv_addr), sizeof(my_tftp->serv_addr));
  my_tftp->serv_addr.sin_family = AF_INET;
  my_tftp->serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_tftp->serv_addr.sin_port = htons(0); // let OS pick randomly

  if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->serv_addr), my_tftp->servlen) < 0)
  {
    printf("%s: can't bind local address\n",my_tftp->progname);
    exit(0);
  }
}

// build client socket
void build_cliSocket(struct tftp *my_tftp, int port)
{
  if(port < 0 || port > 65535){
    port = SERV_UDP_PORT;
  }
  
  my_tftp->type = CLIENT;
  if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
  {
    printf("%s: can't open datagram socket\n",*(my_tftp->progname));
    exit(0);
  }
  // set client address
  bzero((char *) &my_tftp->cli_addr, sizeof(my_tftp->cli_addr));
  my_tftp->cli_addr.sin_family = AF_INET;
  my_tftp->cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_tftp->cli_addr.sin_port = htons(0);
  // set server address
  bzero((char *) &(my_tftp->serv_addr), sizeof(my_tftp->serv_addr));
  my_tftp->serv_addr.sin_family = AF_INET;
  my_tftp->serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
  my_tftp->serv_addr.sin_port = htons(port);
      
  if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->cli_addr), my_tftp->clilen) < 0)
  {
    printf("%s: can't bind local address\n",my_tftp->progname);
    exit(0);
  }
  else
  {
    printf("%s: PORT NUMBER:%d\n",my_tftp->progname,ntohs(my_tftp->serv_addr.sin_port)); // check port num
  }
}

// parse received request
void parse_request(char* fname, char* mode, char* packet)
{
  char *p;
  p = packet + 2;
  strcpy(fname,p); //get FILENAME
  p += strlen(fname) + 1; // 1 is for null 
  strcpy(mode,p); // get MODE
}

// send request
// returns how many bytes it sends
int send_request(struct tftp *my_tftp, int opcode, char* fname, char* mode, char* packet ) 
{
    int snByte;
    char *p; 
    // get opcode 
    if(opcode == RRQ)
    {
        *(short*)packet = htons(RRQ);
        my_tftp->opcode = RRQ;
    } 
    else if (opcode == WRQ)
    {
        *(short*)packet = htons(WRQ);
        my_tftp->opcode = WRQ;
    }
    // get filename 
    p = packet + 2;
    strcpy(p,fname);
    p += strlen(fname) + 1; // 1 is for null
    strcpy(p,mode);
    p += strlen(mode) + 1;

    snByte = p - packet;
    // send packet
    if(sendto(my_tftp->sockfd,packet,snByte,0,(struct sockaddr *) &(my_tftp->serv_addr),my_tftp->servlen) != snByte)
    {
        printf("%s: sendto error on socket\n",my_tftp->progname);
        exit(3);
    } 
    else
    {
      if(opcode == RRQ)
      {
        printf("Sending[Read Request]\n");
      }
      else if(opcode == WRQ)
      {
        printf("Sending[Write Request]\n");
      }
    }

    return snByte;
}

// get response
int get_response(struct tftp *my_tftp, char* packet)
{
  int recByte;
  if(my_tftp->type == SERVER)
  {
    recByte =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->cli_addr),&(my_tftp->clilen));
  }
  else if(my_tftp->type == CLIENT)
  {
    recByte =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->serv_addr),&(my_tftp->servlen));
  }

  return recByte;
}

// get opcode
int get_opcode(struct tftp *my_tftp, char* packet)
{
  int opcode;
  if(ntohs(*(short*) packet) == RRQ)
  {
    opcode = RRQ;
  }
  else if(ntohs(*(short*) packet) == WRQ)
  {
    opcode = WRQ;
  }
  else if(ntohs(*(short*) packet) == DATA)
  {
    opcode = DATA;
  }
  else if(ntohs(*(short*) packet) == ACK)
  {
    opcode = ACK;
  }
    else if(ntohs(*(short*) packet) == ERROR)
  {
    opcode = ERROR;
  }

  return opcode;
}

// send DATA
int send_data(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum)
{
    int snByte1;
    int snByte2;
    *(short*) packet =  htons(DATA);
    my_tftp->opcode = DATA;
    *(short*) (packet +2) = htons(blockNum);

    snByte1 = fread(packet+4,1,512,fp) + 4; // 4byte is for opcode and block number
    if(my_tftp->type == SERVER)
    {
      snByte2 = sendto(my_tftp->sockfd,packet,snByte1,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
    }
    else if(my_tftp->type == CLIENT)
    {
      snByte2 = sendto(my_tftp->sockfd,packet,snByte1,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
    }

    if(snByte1 != snByte2)
    {
        printf("%s: sendto error on socket\n",my_tftp->progname);
        exit(0);            
    }
    else
    {
      if(my_tftp->type == SERVER)
      {
        char dst_addr[INET_ADDRSTRLEN];
        int dst_port;
        inet_ntop(AF_INET,&my_tftp->cli_addr.sin_addr,dst_addr,sizeof(dst_addr));
        dst_port = ntohs(my_tftp->cli_addr.sin_port);
        printf("[UDP/%s:%d] Sending block #%d of data\n",dst_addr,dst_port,blockNum);
      }
      else if(my_tftp->type == CLIENT)
      {
        printf("Sending block #%d of data\n",blockNum);
      }
    }

    return snByte2;
}

// resend DATA; It doesn't continue to read from the file
int resend_data(struct tftp *my_tftp, char* packet,short blockNum)
{
  int snByte1 = strlen(packet+4) + 4;
  int snByte2;
  my_tftp->opcode = DATA;

  if(my_tftp->type == SERVER)
  {
    snByte2 = sendto(my_tftp->sockfd,packet,snByte1 ,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
  }
  else if(my_tftp->type == CLIENT)
  {
    snByte2 = sendto(my_tftp->sockfd,packet,snByte1 ,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
  }

  if(snByte2 != snByte1 )
  {
      printf("%s: sendto error on socket\n",my_tftp->progname);
      exit(0);            
  }
  else
  {
    if(my_tftp->type == SERVER)
    {
      char dst_addr[INET_ADDRSTRLEN];
      int dst_port;
      inet_ntop(AF_INET,&my_tftp->cli_addr.sin_addr,dst_addr,sizeof(dst_addr));
      dst_port = ntohs(my_tftp->cli_addr.sin_port);
      printf("[UDP/%s:%d] Sending block #%d of data\n",dst_addr,dst_port,blockNum);
    }
    else if(my_tftp->type == CLIENT)
    {
      printf("Sending block #%d of data\n",blockNum);
    }
  }
  return snByte2;
}

// send ACK
int send_ack(struct tftp *my_tftp, char* packet, short blockNum)
{
  int snByte;
  *(short*) packet =  htons(ACK);
  my_tftp->opcode = ACK;
  *(short*) (packet +2) = htons(blockNum);

  if(my_tftp->type == SERVER)
  {
    snByte = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
  }
  else if(my_tftp->type == CLIENT)
  {
    snByte = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
  }

  if(snByte != 4)
  {
      printf("%s: sendto error on socket\n",my_tftp->progname);
      exit(3);            
  }
  else
  {
    if(my_tftp->type == SERVER)
    {
      char dst_addr[INET_ADDRSTRLEN];
      int dst_port;
      inet_ntop(AF_INET,&my_tftp->cli_addr.sin_addr,dst_addr,sizeof(dst_addr));
      dst_port = ntohs(my_tftp->cli_addr.sin_port);
      printf("[UDP/%s:%d] Sending Ack# %d\n",dst_addr,dst_port,blockNum);
    }
    else if(my_tftp->type == CLIENT)
    {
      printf("Sending Ack# %d\n",blockNum);
    }
  }

  return snByte;
}

// send ERROR
int send_error(struct tftp *my_tftp, char* packet, short errCode)
{
  int snByte;
  int n = 4;
  *(short*) packet =  htons(ERROR);
  *(short*) (packet +2) = htons(errCode);
  my_tftp->opcode = ERROR;
  char dst_addr[INET_ADDRSTRLEN];
  int dst_port;

  if(errCode == 0) // file not found
  {
    char errmsg[50] = "server: file not found error";
    strcpy(packet+4,errmsg); 
    n += strlen(errmsg) + 1;
  }
  else if(errCode == 1) // file already exist
  {
    char errmsg[70] = "server: file is already exist error(overwrite warning)";
    strcpy(packet+4,errmsg); 
    n += strlen(errmsg) + 1;
  }
  else if(errCode == 2) // no  permission
  {
    char errmsg[50] = "server: no permission to open the file error";
    strcpy(packet+4,errmsg); 
    n += strlen(errmsg) + 1;
  }
  else if(errCode == 3) // filepath 
  {
    char errmsg[50] = "server: a filename is required, not a filepath";
    strcpy(packet+4,errmsg); 
    n += strlen(errmsg) + 1;
  }
  else if(errCode == 4)
  {
    char errmsg[50] = "server: not valid mode request";
    strcpy(packet+4,errmsg); 
    n += strlen(errmsg) + 1;
  }
  snByte = sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
  if(snByte != n)
  {
      printf("%s: sendto error on socket\n",my_tftp->progname);
      exit(3);            
  }
  inet_ntop(AF_INET,&my_tftp->cli_addr.sin_addr,dst_addr,sizeof(dst_addr));
  dst_port = ntohs(my_tftp->cli_addr.sin_port);
  printf("[UDP/%s:%d] Sending ERROR #%d\n",dst_addr,dst_port,errCode);

  return snByte;
}

// retransmit REQ
void retxREQ(struct tftp *my_tftp)
{
    if(count < 10)
    {
        my_tftp->send_req(my_tftp, my_tftp->opcode, my_tftp->fileName, MODE, my_tftp->outStream);
        alarm(TIMEOUT); 
    }
    else
    {
        printf("%s: 10 consecutive timesouts has occurd.\n",my_tftp->progname);
        count = 0;
        fclose(my_tftp->fp);
        my_tftp->fp = NULL;
        exit(0);
    }
}

// retransmit DATA
int retxDATA(struct tftp *my_tftp)
{
    if(count < 10)
    {
        my_tftp->snByte = my_tftp->resend_dta(my_tftp,my_tftp->outStream, my_tftp->blockNum); 
        alarm(TIMEOUT); 
        return 0;
    }
    else
    {
        printf("%s: 10 consecutive timesouts has occurd.\n",my_tftp->progname);
        count = 0;
        alarm(0);
        fclose(my_tftp->fp);
        my_tftp->fp = NULL;
        if(strcmp(my_tftp->progname,"./client") == 0)
        {
          exit(0);
        } 
        return -1;
    }
}

// retransmit ACK
int retxACK(struct tftp *my_tftp)
{
    if(count < 10)
    {
        my_tftp->send_ack(my_tftp, my_tftp->outStream, my_tftp->blockNum );
        alarm(TIMEOUT); 
        return 0;
    }
    else
    {
        printf("%s: 10 consecutive timesouts has occurd.\n",my_tftp->progname);
        count = 0;
        alarm(0);
        fclose(my_tftp->fp);
        remove(my_tftp->fileName); 
        my_tftp->fp = NULL;
        if(strcmp(my_tftp->progname,"./client") == 0)
        {
          exit(0);
        }
        return -1;
    }
}

// alarm handler
void handle_timeout(int signal)
{
    count++;
    printf("Timeout! : %d\n",count);
  //  printf("%d\n",errno);
}

// register alarm handler
int register_handler(struct tftp *my_tftp)
{
    int rt_value = 0;
    /* register the handler function. */
    rt_value = (intptr_t) signal( SIGALRM, my_tftp->handle_timeout );
    if( rt_value == (intptr_t) SIG_ERR ){
        printf("can't register function handle_timeout.\n" );
        printf("signal() error: %s.\n", strerror(errno) );
        return -1;
    }
    /* disable the restart of system call on signal. otherwise the OS will stuck in
     * the system call
     */
    rt_value = siginterrupt( SIGALRM, 1 );
    if( rt_value == -1 ){
        printf( "invalid sig number.\n" );
        return -1;
    }

    return 0;
}

// reset timeout
void timeout_reset()
{
  count = 0;
  alarm(0);
}

