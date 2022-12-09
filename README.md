# CSS432-TFTP_Project

Step-By-Step Instruction to sucessfully run the Server and the Client.

Directory Tree <br/>
```bash
/css432-TFTP_Project (root directory)
    |--tftp 
        |-- makefile
            |--makefile
            |--/client
                |--tftp_client.c
                |--makefile
            |--/common
                |--tftp.h
                |--tftp.c
                |--makefile
            |--/server
                |--tftp_server.c
                |--makefile
```

1. If you are connecting on two different machines which have different IP address
    1. Open the tftp.h file present in the common folder
    2. Change the "SERV_HOST_ADDR" value to your server IP address
2. How to compile the program
    1. Go to makefile directory where the "client", "server", "common" and makefile files are present
    2. Run "make" in the linux command line 
3. How to run the program
    1. Server:  <br/>
       Run "./server" in the "server" directory <br/>
       It will run with a default port# 61124  <br/>
       ---Option--- <br/>
       You can give an option "-p Port#" if you want the server to listen on a specific port number <br/>
       Eg)```$./server -p 12345 ```  <br/>
    2. Client: <br/>
       Your client takes two arguments.  <br/>
       The first indicates whether it is read (-r) or write (-w) opteration  <br/>
       The second is a name of text file to be read or written  <br/>
       Run "./client -r filename.txt or ./client -w filename.txt" <br/>
       It will run with a default server port# 61124 <br/> 
       ---Option--- <br/>
       If you gave a specific Port# to Server, you also must give an option "-p Port#" with the same server Port# <br/>
       Eg)```$./client -p 12345 -r hello.txt ``` or ```$./client -p 12345 -w hello.txt ``` 
