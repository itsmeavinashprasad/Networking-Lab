#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>



void *read_from_server(void *ptr)
{
    int newsockfd,n;
    newsockfd = *((int *)ptr);
    char buffer[256];
    
    while(read(newsockfd,buffer,255)>0)
    {
        printf("Server: %s", buffer);
        bzero(buffer,256);
    }
}

void write_to_server(int newsockfd)
{
    char buffer[256];
    while(1)
    {
        bzero(buffer,256);
        fgets(buffer,255,stdin);

        write(newsockfd,buffer,strlen(buffer));

        //terminating condition
        if(strstr(buffer, "quit") != NULL)
            break;
        
    }
}
int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    sscanf(argv[1], "%d", &portno);

    struct sockaddr_in serv_addr;

    // ====================== socket() call ===============================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("Failed in Creating Socket\n");
        exit(0);
    }
    // ====================== connect() call ===============================
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address, Address can not be reached \n");
        exit(0);
    }
    
    if( connect( sockfd ,(struct sockaddr *) &serv_addr , sizeof(serv_addr) ) == -1 )
    {
        printf("Failed in connecting to server\n");
        exit(0);
    }
    printf("Connected to server\n");

    // ================================ read and write ==========================
    printf("=============================================\n");
    pthread_t rd_t, wr_t;

    pthread_create( &rd_t, NULL, read_from_server,  &sockfd );

    write_to_server(sockfd);

	//================close all sockets=================================
    close(sockfd);
	printf("Quitting\n");

    return 0;
}
