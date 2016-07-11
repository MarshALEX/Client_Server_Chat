/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   
   usage: server [port number 30020 to 65535  ]
   
   Alex Marsh
 04/26/2016
 CS 372 Project 1
 written in C++
 
 This project pairs with chatclient.c and implements a client-server network application using the sockets API adn TCP protocol. 
 
 chatserve will start on host A and wait on a port specified by command line. chatclient will start on hostB and specify host A's hostname ('localhost' if on same computer) and port number from the command line. 
 
 Code taken from resources: Linux How To: Socket Tutorial and Beej's Guide to Network Programming
            http://www.linuxhowtos.org/C_C++/socket.htm
            http://beej.us/guide/bgnet/output/html/multipage/index.html
   
   */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>


#define MAXDATASIZE 550 //max number of bytes we can get at once
int setUp(int argc, char *argv[], struct sockaddr_in serv_addr, struct sockaddr_in cli_addr);
void error(const char *msg);
void buildMessage(char* clientMsg, char* name, int handleLength, char* buffer);
void chat(char* clientMsg, char* buffer, int newsockfd, char* serverHandle, char* serverMsg, int handleLen);

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[MAXDATASIZE];
     char clientMsg[MAXDATASIZE];
     char serverMsg[MAXDATASIZE];
     char *serverHandle = "ServerMan";
     int handleLen = strlen(serverHandle);
     int pid;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     
     sockfd = setUp(argc, argv, serv_addr, cli_addr);
    
     clilen = sizeof(cli_addr); //get size (# of bytes) of client struct
     
    while(1){
        signal(SIGCHLD, SIG_IGN); //ignore the SIGCHLD signal

        //connect to pending connection and make a new socket fd for this connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
        if (newsockfd < 0) //error check accept
            error("ERROR on accept");
     
        pid = fork();
        if(pid < 0)
            error("ERROR on fork");
        if(pid == 0){ //in the child process
            close(sockfd); //close socket
            //call chat to send and recieve messages
            chat(clientMsg, buffer, newsockfd, serverHandle, serverMsg, handleLen);
            printf("Client has left Chat\n");
            printf("In server waiting...listening...watching...\n");
            exit(0);
                
        }
        else //we are in the parent process
            close(newsockfd);
     }
     close(sockfd);
     return 0;  //should never reach here
}


/*
*           setUp
* Description: This function sets up an open socket to accept clients
* Input: the number of arguments from the command line, the array holding the arguments
            from teh command line, the two struct sockaddr_in objects for the server and client 
* Output: The integer of the open socket. 
*/

int setUp(int argc, char *argv[], struct sockaddr_in serv_addr, struct sockaddr_in cli_addr){
    int sockfd, portno;
    
    //check usage
     if (argc < 2) {
         printf("usage: server [port number]\n");
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
    
     //create a socekt
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) //error check socket
        error("ERROR opening socket");
    
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]); //get port number server will use
     serv_addr.sin_family = AF_INET; //host byte order
     serv_addr.sin_addr.s_addr = INADDR_ANY; //use my IP address
     serv_addr.sin_port = htons(portno); //short network byte order
     
    
    //associate the socket with the port number with error checking
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
    
     if((listen(sockfd,5)) == -1)//listen for someone
         error("ERROR listening"); //error check listening
     
     printf("In server waiting...listening...watching...\n");
     
    return sockfd;  //return the open socket
}

/*
*           error
* Description: This function prints an error message
* Input: char array of message
* Output: None
*/
void error(const char *msg)
{
    perror(msg);
    exit(1);
}


/*
*           buildMessage
* Description: This function creates a message to send to the server.  The function takes the
*       users name and inserts it in the begining of the buffer.  Then appends the symbol '>'
        to initiat a prompt. The clients message is then appended to the buffer. 
        The end result in buffer will resemble:
                [clientsName]> [clients message]
* Input: char array of message from client, clients name, length of clients name, and array
*           to insert final message in
* Output: None
*/
void buildMessage(char* clientMsg, char* name, int handleLength, char* buffer){
    char* symbol = "> ";
   
    //copy users name into buffer
    strncat(buffer, name, handleLength);
    
    //append symbol to name
    strncat(buffer, symbol, 3);

    //append users message to name and symbol
    strncat(buffer, clientMsg, 500);

}

/*
*           chat
* Description: This function contains a loop that allows the server to remain open as long as the client
        holds a connection with the server. Once the client closes connection the server returns to main and 
        closes the connection.  This function recieves the clients message, prints the message and then prompts the
        server to input a message. The servers message is then sent to the client and the server waits for the
        next message to be recieved. 
* Input: char array of message from client, buffer that holds the input from server, socketfile descriptor, 
        the name of the server, the character array to hold the servers message, and the length of the 
        servers handle.
* Output: None
*/

void chat(char* clientMsg, char* buffer, int newsockfd, char* serverHandle, char* serverMsg, int handleLen)
{
    
    int clientHere = 1;  //set flag to true
    int bytes_recv, bytes_sent, len;
  
    while(clientHere == 1){
          
            memset(clientMsg, 0, MAXDATASIZE);
            memset(buffer, 0, MAXDATASIZE);

            //recieve message and erro check
            if((bytes_recv = recv(newsockfd, clientMsg, MAXDATASIZE-1, 0)) <= 0){
                if(bytes_recv == 0){ //client closed the connection
                    clientHere = 0;
                }
                else  //there is an error connecting
                    error("ERROR recieving from socket\n");
            }
            else{  //get data from client
            printf("%s\n", clientMsg); //print clients message

            printf("%s> ", serverHandle);
            bzero(serverMsg,MAXDATASIZE);
            fgets(serverMsg,MAXDATASIZE,stdin);//get user's message to send to server

            bzero(buffer,MAXDATASIZE);

            //prepend handle to message
            buildMessage(serverMsg, serverHandle, handleLen, buffer);

            len = strlen(buffer);
            //send message to client
            bytes_sent = send(newsockfd, buffer, len, 0);
            if(bytes_sent == -1)
                error("ERROR sending to socket\n");
            }
    }
            
}