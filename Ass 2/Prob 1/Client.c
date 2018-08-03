#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    sscanf(argv[1], "%d", &portno);

    struct sockaddr_in serv_addr;
    // struct hostent *server;
    char buffer[256];
    // ====================== socket() call ===============================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("Failed in Creating Socket\n");
        exit(0);
    }
    // ============================= connect() =============================
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address, Address can not be reached \n");
        exit(0);
    }
    
    if( connect( sockfd ,(struct sockaddr *) &serv_addr , sizeof(serv_addr) ) == -1 )
    {
        printf("Failed in to connect to server\n");
        exit(0);
    }
    printf("Connected to server\n");

    // ================================ read and write ==========================
    while(1)
    {
    	printf("-- > ");
	    bzero(buffer,256);
	    fgets(buffer,255,stdin);

	    n = write(sockfd,buffer,strlen(buffer));
	    if ( strstr(buffer, "quit") != NULL)
	    	break;
	    bzero(buffer,256);
	    n = read(sockfd,buffer,255);
	    printf("Server: %s\n",buffer);
	}
    close(sockfd);
	printf("Quitting\n");

    return 0;
}
