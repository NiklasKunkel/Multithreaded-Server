/*
 * Niklas Kunkel
 * Niklas@theKunkels.com
 * 04/17/14
 *
 *This is a multithreaded server. Every time a client connects the server
 * a new thread is designated to that connection. 
 *
 * After connection from client, first input from client will be a printed string sent to the server.
 * Server will send the client the total amount of clients connected as well as the amount
 * of time the server has been active.
 
 * After this first string, every integer input afterwards will be summated to a
 * cumulative sum and then returned to the client.
 *
 *To Run This Server: 
 *      1.Compile multi_server.c in gcc
 *      2. Execute in bash: ./multi_server [port #]
 *      3. The server is now listening for connections from clients on [port #]
 *
 *To Connect to this Server:
 *      1. From same device:
 *          a) In bash: telnet localhost [port #]
 *
 *      2. From external device:
 *          a) In bash: telnet [ip] [port #]
 */

#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

time_t current_time;
time_t start_time;
pthread_t thread;

//Holds all information for a single connected client
typedef struct
{
    int sock;                   //the socket of the connection to one client
    struct sockaddr address;    //the address of a connected client
    int addr_len;               //length of the address field
} connection_t;

void * process(void * ptr)      //Pass in a pointer to our connection structure
{
    char *buffer;
    char *run_time;
    char *int_prompt;
    int len;
    connection_t *connection;       //pointer to our connection structure
    int addr = 0;
    static int total_clients;       //stores the total number of clients since server startup
    int sum = 0;                    //stores running sum of all numbers entered by user
    int n1,n2;                      //error check for succesful write to client
    int num_in;                     //stores number input from client
    //pthread_id_np_t tid;            //stores thread ID - was removed because gcc said 'undeclared identifier'
    n1 = n2 = -1;

    if(!ptr) pthread_exit(0);           //if ptr is invalid, exit thread
    connection = (connection_t *)ptr;   //cast ptr into a connection_t
    
    total_clients++;
    
    //Read the Length of the Message from Client
    read(connection->sock, &len, sizeof(int));  //Syntax: (socket, message, size of message)
    if(len > 0)
    {
        //Obtain The Clients IP Address
        addr = (long)((struct sockaddr_in *)&connection->address)->sin_addr.s_addr;
        
        //Allocate a buffer in memory for the message
        buffer = (char *)malloc((len+1)*sizeof(char));
        run_time = (char *)malloc((len+1)*sizeof(char));
        
        //Ends buffer after end of message
        buffer[len] = 0;
        
        //Read Message
        read(connection->sock, buffer, len);
        
        //Print Message
        
        printf("%d.%d.%d.%d: %s\n",
               (addr)       & 0xff,
               (addr >> 8)  & 0xff,
               (addr >> 16) & 0xff,
               (addr >> 24) & 0xff,
               buffer);
        
        
        //Print Total # of Clients to Server
        printf("Total number of clients connected = %d\n", total_clients);
        
        //Obtain server uptime
        current_time = time(NULL);
        current_time = difftime(current_time,start_time);
        
        //Print Total Server UpTime to Server
        printf("Total server uptime = %d seconds \n\n",(int)current_time);
        
        //Create the strings we will send to the client
        sprintf(buffer, "Total number of clients connected = %d \n",total_clients);
        sprintf(run_time, "Total server uptime = %d seconds \n\n Please enter integers one by one: Server will return cumulative sum: \n",(int)current_time);
        
        //Write Total Clients Connected & Total Server Uptime to Client
        n1 = write(connection->sock, buffer, strlen(buffer));
        n2 = write(connection->sock, run_time, strlen(run_time));
        if(n1 < 0 || n2 < 0)
        {
            fprintf(stderr, "error: Sending to client failed\n");
        }
        
        //Let client continuously enter integers and have the server send cumulative sum to client
        do
        {
            n1 = n2 = -1;
            num_in = -1;
            
            //Read input from client
            n1 = read(connection->sock, buffer, strlen(buffer));
            num_in = atoi(buffer);
            
            if(n1 < 0)
            {
                fprintf(stderr, "Error: receiving input from client failed\n");
                break;
            }
            
            //pthread_getunique_np(&thread, &tid);  //removed because gcc couldnt find
            
            //Print input from client to Server
            printf("Num_In = %d \n",num_in);
            
            //Increment sum by client input
            sum += num_in;
            
            
            printf("New Sum = %d \n", sum);
            
            //Send cumulative sum to client
            sprintf(buffer, "The Cumulative Sum = %d \n Enter negative number to exit \n\n", sum);
            n2 = write(connection->sock, buffer, strlen(buffer));
            if(n2 < 0)
            {
                fprintf(stderr, "Error: sending to client failed\n");
                break;
            }
        }while(num_in >= 0);
        
        
        
        free(buffer);
    }
    
    //Close Socket and Clean-Up
    close(connection->sock);
    
    //Frees the memory
    free(connection);
    
    //Exit Thread
    pthread_exit(0);
}

int main(int argc, char ** argv)
{
    int sock = -1;                  //the socket (used to error check socket create)
    struct sockaddr_in address;     //
    int port;                       //the port number to host the server on
    connection_t *connection;       //struct that holds all information for one client connection
    //pthread_t thread;
    
    //Parse and error-check command line args
    if(argc != 2)
    {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        return -1;
    }
    
    //Obtain port number
    if(sscanf(argv[1], "%d", &port) <= 0)
    {
        fprintf(stderr, "%s: error: wrong parameter: port \n", argv[0]);
        return -2;
    }
    
    //Create Socket - if successful sock -> positive number
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock <= 0)
    {
        fprintf(stderr, "%s: error: cannot create socket\n", argv[0]);
        return -3;
    }
    
    //Bind Socket To Port 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if(bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
    {
        fprintf(stderr, "%s: error: cannot bind socket to port %d\n", argv[0], port);
        return -4;
    }
    
    //Listen on port and wait for client connection
    if(listen(sock, 5) < 0)
    {
        fprintf(stderr, "%s: error: cannot listen on port\n", argv[0]);
        return -5;
    }
    
    printf("%s: server ready and listening for connections on port %d\n", argv[0], port);
    
    //Save time server started running
    start_time = time(NULL);
    
    while(1)
    {
        //Accept Incoming Connections
        connection = (connection_t *)malloc(sizeof(connection_t));
        connection->sock = accept(sock, &connection->address, &connection->addr_len);
        
        if(connection->sock <= 0)
        {
            free(connection);
        }
        
        else
        {
            //Start a new thread but do not wait for it
            pthread_create(&thread, 0, process, (void *)connection);
            pthread_detach(thread);
        }
    }
    
    return 0;
}