server.out : tftp_server.o 
	gcc -lpthread tftp_server.o -L../common -ltftp -o server

tftp_server.o :
	gcc -c tftp_server.c 