#include "tftp.h"

// implementation
struct tftp* tftp_init(char* name) 
{
	struct tftp *my_tftp = malloc(sizeof(struct tftp));
  my_tftp->build_cli = build_cliSocket;
  my_tftp->build_serv = build_servSocket;
  my_tftp->parse_req = parse_request;
  my_tftp->send_req = send_request;
  my_tftp->send_dta = send_data;
  my_tftp->send_ack = send_ack;
  my_tftp->get_resp = get_response;
  my_tftp->get_opc = get_opcode;
  
  my_tftp->sockfd = 0;
  my_tftp->clilen = sizeof(my_tftp->cli_addr);
  my_tftp->servlen = sizeof(my_tftp->serv_addr);
  my_tftp->progname = name;
  my_tftp->type = 0;
	return my_tftp;
}

// build server socket
void build_servSocket(struct tftp *my_tftp, int port)
{
    if(port <= 0 && port >= 65535){
      port = SERV_UDP_PORT;
    }
    
    my_tftp->type = SERVER;
    if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
       	printf("%s: can't open datagram socket\n",*(my_tftp->progname));
		exit(1);
    }
    bzero((char *) &(my_tftp->cli_addr), sizeof(my_tftp->cli_addr));
    my_tftp->cli_addr.sin_family = AF_INET;
    my_tftp->cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_tftp->cli_addr.sin_port = htons(port);
        
    if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->cli_addr), my_tftp->clilen) < 0)
    {
		printf("%s: can't bind local address\n",my_tftp->progname);
		exit(2);
    }
}

// build client socket
void build_cliSocket(struct tftp *my_tftp, int port)
{
    if(port <= 0 && port >= 65535){
      port = SERV_UDP_PORT;
    }
    
    my_tftp->type = CLIENT;
    if((my_tftp->sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
       	printf("%s: can't open datagram socket\n",*(my_tftp->progname));
		exit(1);
    }
    // cli addr
    bzero((char *) &my_tftp->cli_addr, sizeof(my_tftp->cli_addr));
    my_tftp->cli_addr.sin_family = AF_INET;
    my_tftp->cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_tftp->cli_addr.sin_port = htons(0);
    // serv addr
    bzero((char *) &(my_tftp->serv_addr), sizeof(my_tftp->serv_addr));
    my_tftp->serv_addr.sin_family = AF_INET;
    my_tftp->serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    my_tftp->serv_addr.sin_port = htons(port);
        
    if(bind(my_tftp->sockfd, (struct sockaddr *) &(my_tftp->cli_addr), my_tftp->clilen) < 0)
    {
		printf("%s: can't bind local address\n",my_tftp->progname);
		exit(2);
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
int send_request(struct tftp *my_tftp, char* opc, char* fname, char* mode, char* packet ) 
{
    int n;
    char *p; 
    // get opcode 
    if(opc[1] == 'r')
    {
        *(short*)packet = htons(RRQ);
    } 
    else if (opc[1] == 'w')
    {
        *(short*)packet = htons(WRQ);
    }
    // get filename 
    p = packet + 2;
    strcpy(p,fname);
    p += strlen(fname) + 1; // 1 is for null
    strcpy(p,mode);
    p += strlen(mode) + 1;

    n = p - packet;
    // send packet
    if(sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *) &(my_tftp->serv_addr),my_tftp->servlen) != n)
    {
        printf("%s: sendto error on socket\n",my_tftp->progname);
        exit(3);
    }
    return n;
}

// get response
int get_response(struct tftp *my_tftp, char* packet)
{
  int n;
  if(my_tftp->type == SERVER)
  {
    n =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->cli_addr),&(my_tftp->clilen));
  }
  else if(my_tftp->type == CLIENT)
  {
    n =  recvfrom(my_tftp->sockfd,packet,SSIZE,0,(struct sockaddr *) &(my_tftp->serv_addr),&(my_tftp->servlen));
  }

  if (n < 0)
  {
      printf("%s: recvfrom error\n",my_tftp->progname);
      exit(4);
  }
  return n;
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
  else if(ntohs(*(short*) packet) == PORT){
    opcode = PORT;
  }
  return opcode;
}

// send DATA
int send_data(struct tftp *my_tftp, FILE* fp, char* packet, short blockNum)
{
    int n;
    int sn;
    *(short*) packet =  htons(DATA);
    *(short*) (packet +2) = htons(blockNum);
    n = fread(packet+4,1,512,fp) + 4; // 4byte is for opcode and block number
    if(my_tftp->type == SERVER)
    {
      sn = sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
    }
    else if(my_tftp->type == CLIENT)
    {
      sn = sendto(my_tftp->sockfd,packet,n,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
    }    
    if(sn != n)
    {
        printf("%s: sendto error on socket\n",my_tftp->progname);
        exit(3);            
    }
    else
    {
        printf("Sending block #%d of data\n",blockNum);
    }
    return sn;
}

// send ACK
int send_ack(struct tftp *my_tftp, char* packet, short blockNum)
{
  int n;
  *(short*) packet =  htons(ACK);
  *(short*) (packet +2) = htons(blockNum);

  if(my_tftp->type == SERVER)
  {
    n = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->cli_addr),my_tftp->clilen);
  }
  else if(my_tftp->type == CLIENT)
  {
    n = sendto(my_tftp->sockfd,packet,4,0,(struct sockaddr *)&(my_tftp->serv_addr),my_tftp->servlen);
  }

  if(n != 4)
  {
      printf("%s: sendto error on socket\n",my_tftp->progname);
      exit(3);            
  }
  else
  {
      printf("Sending Ack# %d\n",blockNum);
  }
  return n;
}
