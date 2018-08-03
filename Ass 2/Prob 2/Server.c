#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void read_from_client(int newsockfd)
{
    char buffer[256];
    puts("In read function");
    while(read(newsockfd,buffer,255)>0)
    {
        //terminating condition
        if(strstr(buffer, "quit") != NULL)
            break;
        
        printf("Client: %s", buffer);
        bzero(buffer , 256);
    }
    return ;
}

void *write_to_client(void *ptr)
{
    int newsockfd, n;
    newsockfd = *((int *)ptr);
    char buffer[256];
    while(1)
    {
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        
        n = write(newsockfd,buffer,strlen(buffer));
    }
}

int main(int argc, char const *argv[])
{
    int sockfd, newsockfd, portno, clilen, n, cli_id=1;
    // sockfd is socket file desc
    // newsockfd is client file desc
    sscanf(argv[1],"%d",&portno);
    // clilen is client address length
    // n is no of bytes sent or received

    char buffer[256];
    // the buffer in which msg is stored

    struct sockaddr_in serv_addr, cli_addr;
    // sockaddr_in family where : serv_addr and cli_addr contains server and client add respectively


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

    if( listen(sockfd,3) == -1)
    {
        printf("Fail in listen() call \n");
        exit(0);
    }
    printf("Listening for servers\n");

    // =============================== accept() call ===============================
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    // accept return client's add details 
    
    
    // if all goes fine untill now the the connection is established
    printf("Connection Established \n");


    // =================================== Ready to send and recv data =========================
    printf("=============================================\n");

    pthread_t rd_t, wr_t;


    pthread_create( &wr_t, NULL, write_to_client, &newsockfd );

    read_from_client(newsockfd);

    // ====================== close all sockets ===========================
    close(newsockfd);
    close(sockfd);
	printf("Server Closed\n");
	
	return 0;
}
