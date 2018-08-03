#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PACKET_SIZE 1024
#define _printBuffer 1

int main(int argc, char *argv[])
{
	int sockfd, portno, bytecount;
	struct sockaddr_in serv_addr;
	struct sockaddr_storage addr;
	socklen_t fromlen;

	//take port no
	sscanf(argv[1], "%d", &portno);

	// socket() call
	sockfd = socket(AF_INET , SOCK_DGRAM , 0);
	if(sockfd == -1)
	{
		printf("Failed in creating socket. Exiting...\n" );
		exit(0);
	}
	else
		printf("Socket Created ...\n");

	// assigning values
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	// serv_addr.sin_addr.s_addr = inet_addr("10.2.90.207");
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// bind() call
	if( bind( sockfd , (struct sockaddr * ) &serv_addr , sizeof(serv_addr)) == -1)
	{
		printf("Failed to bind() \n");
		exit(0);
	}
	else
		printf("Successful in bind() call\n");

	fromlen = sizeof(addr);
	unsigned char buffer[DEFAULT_PACKET_SIZE];	// buffer used for sending and receiving
	int ttl = 0;
	while(1)
	{
		//  ============================= RECEIVE PACKET =======================================
		bzero(buffer , DEFAULT_PACKET_SIZE);
		bytecount = recvfrom(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &addr , &fromlen);
		if(bytecount == -1)
			printf("Error in receiving data\n");

		if(_printBuffer)
		{
			for(int j=0; j<20; j++)
				printf("%X ", buffer[j]);
			printf("\n===============================\n");

			printf("REVEIVED: %d BYTES\n",bytecount );
		}
		
		// ============================= PROCESSING DATA ==============================================
		memcpy(&ttl, buffer+6, 1);
		// ttl = ntohs(ttl);
		ttl--;
		// ttl = htons(ttl);
		memcpy(buffer+6, &ttl, 1);
		ttl = 0;
		// ================================== SEND PACKET =============================================
		bytecount = sendto(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &addr, sizeof(addr));
	}
	return 0;
}