#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CL 30
#define _printStat 1
#define _printMsg 0
 
struct Client_info
{
    struct sockaddr_in cli_addr;
    char name[20];
    // int sender;
    int sockfd;
    pthread_t rd_t;
};

typedef struct Client_info Client_info;

int *client_status = NULL;
Client_info *clients = NULL;
int no_of_clients=0;
int close_var = 0;

void send_to_receipient(int sender, char *receipient, char *msg)
{
    char buffer[260];
    bzero(buffer , 260);
    for(int i=0; i<no_of_clients; i++)
    {

        if(client_status[i] == 0)
        {
            //  if i-th client is inactive
            continue;
        }
        else if(strcmp( clients[i].name , receipient) == 0)
        {
            if( i == sender)
            {
                // when sender wants to send himself
                strcat(buffer , "SERVER: Why are you trying send to yourself\n");
                write(clients[sender].sockfd , buffer , strlen(buffer));
                return;
            }
            // receipient found
            strcpy(buffer , clients[sender].name);
            strcat(buffer , " : ");
            strcat(buffer , msg );
            write(clients[i].sockfd , buffer , strlen(buffer));
            return;
        }
    }
    strcpy(buffer , receipient);
    strcat(buffer , " NOT FOUND\n");
    write(clients[sender].sockfd , buffer, strlen(buffer));
    return;
}

void *read_from_each_client(void *ptr)
{
    char buffer[256], str[260] , str2[260] , str3[260];
    int sender = *((int *) ptr);
    
    if(_printStat)
    {
        strcpy(str , ">>>>>>>>>>>> ");
        strcat(str , clients[sender].name);
        strcat(str , " Joined Chat Room <<<<<<<<<<\n");
        printf("%s",str );
        bzero(str, 260);
    }
    
    char *index_ptr;
    
    int index=0;
    while(read(clients[sender].sockfd , str , 259)>0)
    {
        //terminating condition
        if(strstr(str, "quit") != NULL)
        {
            client_status[sender] = 0;
            break;
        }
        index_ptr = strchr(str , ':');
        index = (int) (index_ptr - str+1);
        
        strncpy(str2 , str , index - 2);
        strcpy(str3 , (str+index+1));
        
        if(_printMsg)
            printf("%s to %s : %s\n",clients[sender].name , str2 , str3 );
        
        send_to_receipient(sender , str2 , str3);

        // reset all strings
        bzero(str,260);
        bzero(str2,260);
        bzero(str3,260);
        printf("All set to zero\n" );
    }
    bzero(str , 260);
    client_status[sender] = 0;
    if(_printStat)
    {
        strcpy(str , ">>>>>>>>>>>> ");
        strcat(str , clients[sender].name);
        strcat(str , " Left Chat Room <<<<<<<<<<\n");
        printf("%s",str );
    }
    
}

void *close_cond()
{
    char buffer[260];
    fgets(buffer , 255 , stdin);
    if(strstr(buffer , "close") != NULL)
    {
        for(int i=0; i<no_of_clients; i++)
        {
            if(client_status[i] == 0)
                continue;
            write(clients[i].sockfd , buffer , strlen(buffer) );
        }
    }
    close_var = 1;
}


int main(int argc, char const *argv[])
{
    int sockfd, portno, clilen, n, cli_id=1;
    // get port no
    sscanf(argv[1], "%d", &portno);
    struct sockaddr_in serv_addr, cli_addr;


    // ====================== socket() ===============================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("Failed in Creating Socket\n");
        exit(0);
    }
    // AF_INET = is for IPv4
    // SOCK_STREAM is for TCP

    printf("Socket Made\n");

    // ========================== bind() call =================================
    // initialising serv_addr values
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  

    if( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Faild to bind() \n");
        exit(0);
    }
    printf("Succed in bind() call\n");

    // ============================= listen() call ==================================

    if( listen(sockfd,MAX_CL) == -1)
    {
        printf("Fail in listen() call \n");
        exit(0);
    }
    printf("Listening for servers\n");

    // =============================== accept() call ===============================

    
    Client_info temp;
    clilen = sizeof(temp.cli_addr);
    char name[20];

    clients = (Client_info*) malloc( sizeof( Client_info ));
    client_status = (int*) malloc( sizeof(int));

    pthread_t close_t;
    pthread_create(&close_t, NULL, close_cond, NULL);
    int i;
    while(no_of_clients<MAX_CL)
    {
        i=no_of_clients;
        no_of_clients++;

        // increasing array size
        clients = (Client_info*) realloc(clients , no_of_clients * sizeof(Client_info));
        client_status = (int*) realloc(client_status , no_of_clients * sizeof(int));
        
        clients[i].sockfd = accept(sockfd, (struct sockaddr *) &clients[i].cli_addr, &clilen);
        client_status[i] = 1;
        
        read(clients[i].sockfd , clients[i].name , 19);
        
        // making newlinw character to NULL
        n = strlen(clients[i].name);
        clients[i].name[n-1] = '\0';

        int temp = i;
        pthread_create( &clients[i].rd_t, NULL, read_from_each_client,  &temp );
        char usage[] = "FORMAT: <receipient> <space> : <space> <message>";
        write(clients[i].sockfd, usage, sizeof(usage));
    }
    
    pthread_join(close_t , NULL);

    close(sockfd);    
	printf("Server Closed\n");
	
	return 0;
}
