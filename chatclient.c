/*
 chatclient.c
 
 usage: 
    client [hostname] [port number]
    ex: client cheerios 51717
 
 Alex Marsh
 04/26/2016
 CS 372 Project 1
 written in C
 
 This project pairs with chatserve.c and implements a client-server network application using the sockets API adn TCP protocol. 
 
 chatserve will start on host A and wait on a port specified by command line. chatclient will start on hostB and specify host A's hostname ('localhost' if on same computer) and port number from the command line. 
 
Code taken from resources: 
Linux How To: Socket Tutorial 
            http://www.linuxhowtos.org/C_C++/socket.htm
Beej's Guide to Network Programming       

    http://beej.us/guide/bgnet/output/html/multipage/index.html          
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define MAXDATASIZE 550 //max number of bytes we can get at once

void error(const char *msg);
void buildMessage(char* clientMsg, char* name, int handleLength, char* buffer);
void getClientHandle(char* clientHandle);
void getClientMessage(char* clientHandle, char* clientMsg);
void chat(char* clientHandle, char* clientMsg, char* buffer, int sockfd, char* endCommand);
int setUp(int argc, char *argv[], struct sockaddr_in my_addr, struct hostent *server);

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in my_addr; //my address information
    struct hostent *server;
    char buffer[MAXDATASIZE];
    char clientMsg[MAXDATASIZE];
    char endCommand[] = "\\quit";
    
    char clientHandle[MAXDATASIZE];
    int handleLen;  //length of users handle
     
    sockfd = setUp(argc, argv, my_addr, server);

    
    //get client's Handle
    getClientHandle(clientHandle);
    //get client's message
    getClientMessage(clientHandle, clientMsg);

    chat(clientHandle, clientMsg, buffer, sockfd, endCommand);
    
    close(sockfd); //close connection
    return 0;
}

/*
*           setUp
* Description: This function opens a socket to connect to the server
* Input: the number of arguments from the command line, the array holding the arguments
            from teh command line, the struct sockaddr_in object, and struct hostent object
* Output: The integer of the open socket. 
*/

int setUp(int argc, char *argv[], struct sockaddr_in my_addr, struct hostent *server){
    
    int sockfd, portno, n;

    
    //check usage
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    //get port number user wants to connect to
    portno = atoi(argv[2]); 
    
    //get server user wants to connect to
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //create a socket
    if (sockfd < 0) //error check socket
        error("ERROR opening socket");
    
    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET; //host byte order
    //copy memcpy(desitnation, source, size)
    memcpy((char *)&my_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    my_addr.sin_port = htons(portno); //short network byte order
    
    //connect to server and check it it not -1
    if (connect(sockfd,(struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) 
        error("ERROR connecting");

    return sockfd;
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
    exit(0);
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
    
    handleLength = handleLength - 1;

    char* symbol = "> ";
    
    //copy users name into buffer
    strncat(buffer, name, handleLength);
    
    //append symbol to name
    strncat(buffer, symbol, 3);
    
    //append users message to name and symbol
    strncat(buffer, clientMsg, 500);

}

/*
*           getClientHandle
* Description: This function prompts the user for a handle to be used
*       It error checks that the users handle is no longer than 10 charactesr long
* Input: char array that holds the handle name of the client
* Output: None
*/
void getClientHandle(char* clientHandle){
    int handleLen; 
    //get user's handle
    printf("What should I call you? Please enter your handle: ");
    //bzero(clientHandle, 11);
    fgets(clientHandle, MAXDATASIZE, stdin); //get users handle
    handleLen = strlen(clientHandle);
    //error check name is no longer than 10 letters
    while(handleLen > 11){
        printf("Sorry, your handle is too long. Use a maximum of 10 characters please.\n");
        //get user's handle
        printf("What should I call you? Please enter your handle: ");
        bzero(clientHandle, MAXDATASIZE);
        fgets(clientHandle, MAXDATASIZE, stdin); //get users handle
        handleLen = strlen(clientHandle);    
    }
}

/*
*           getClientMessage
* Description: This function prompts the user for a message to send to the server
*           The user is prompted by their handle and the > symbol. The input is then stripped
*            of the new line and a null terminator is then used in it's place. 
* Input: char array that holds the handle name of the client and character array to 
*           hold clients message
* Output: None
*/

void getClientMessage(char* clientHandle, char* clientMsg){
    int handleLen;
    //remove null from clientHandle length count
    handleLen = strlen(clientHandle) - 1; 
    
    // prompt user for message
    printf("%.*s> ", handleLen, clientHandle);
    bzero(clientMsg,MAXDATASIZE);
    fgets(clientMsg,MAXDATASIZE,stdin);//get user's message to send to server
   
    //strip off newline char from clientMsg
    size_t ln = strlen(clientMsg) - 1;
    if(clientMsg[ln] == '\n')
        clientMsg[ln] = '\0';
}

/*
*           chat
* Description: This function contains a loop that allows teh user to recieve and send
            messages to and from the server.  The loop ends when the user enters '\quit'
* Input: char array that holds the handle name of the client, character array to 
*           hold clients message, char array buffer to hold the recieved message, the socket
            file descriptor, and the char array that holds the phrase '\quit' to compare to 
            the user input.
* Output: None
*/

void chat(char* clientHandle, char* clientMsg, char* buffer, int sockfd, char* endCommand){
    int bytes_sent, bytes_recv; //will be returned from send and recieve
    int len; // length of message from user 
    int handleLen;
    
    handleLen = strlen(clientHandle); //get length of clients handle
    while(strcmp(clientMsg, endCommand) != 0){

        bzero(buffer,MAXDATASIZE);

        //prepend handle to message
        buildMessage(clientMsg, clientHandle, handleLen, buffer);

        len = strlen(buffer); //get length of users message

        //send message to server
        bytes_sent = send(sockfd, buffer, len, 0);
        if(bytes_sent == -1)
            error("ERROR sending to socket\n");
        
        bzero(buffer,MAXDATASIZE);

        //recieve message and erro check
        if((bytes_recv = recv(sockfd, buffer, MAXDATASIZE-1, 0)) == -1)
            error("ERROR recieving from socket\n");

        buffer[bytes_recv] = '\0'; //append null to end of string
        
        printf("%s",buffer); //print recieved message
        
        //get client's message
        getClientMessage(clientHandle, clientMsg);
    }
}
