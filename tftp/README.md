Step-By-Step Instruction to sucessfully run the Server and the Client.
1. If you are connecting on two different machines which have different IP address
    1. Open the tftp.h file present in the common folder
    2. Change the "SERV_HOST_ADDR" value to your server IP address
    3. Go back to the main folder where the "client", "server", "common" and makefile files are present
    4. Run "make" in the linux command line for both the client and server machines
    5. Access the client folder on the client machine and the server folder on the server machine
    6. On the server, run "./server" and you can add "-p Port#" if you want the server to listen on a specific port number
    7. On the client, run "./client", you can add "-p Port#" to listen on the same port number as the server. Also you have to include the action the client must do 
      which are read (-r) or write (-w) with a text file.
2. If you are using the same ip address for both the client and server machines then do steps 3-7
