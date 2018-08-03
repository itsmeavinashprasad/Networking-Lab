#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define SIZE 20
#define MAX_UNSIGNED_INT 4294967295 		// value of 2^32 - 1, used for masking
#define _printRTT 0
#define _printBuffer 0

int main(int argc, char *argv[])
{
	//  declaring variable that enterd through cmd line
	char server_ip[16] ;
	int sockfd, portno, payload, ttl, num_packets;
	struct sockaddr_in serv_addr;
	socklen_t fromlen;


	// take inputs
	sscanf(argv[1] , "%s", server_ip);
	sscanf(argv[2] , "%d", &portno);
	sscanf(argv[3] , "%d", &payload);
	sscanf(argv[4] , "%d", &ttl);
	sscanf(argv[5] , "%d", &num_packets);

	// socket call
	sockfd = socket(AF_INET , SOCK_DGRAM , 0);
	if(sockfd == -1)
	{
		printf("Failed in creating socket. \n");
		exit(0);
	}

	// assigning values 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);

	fromlen = sizeof(serv_addr);
	
	int size = 7 + payload;				// size of the packet
	unsigned char buffer[size];			// buffer used for sending and receiving
	unsigned short int seq_no;			// used for storing seq no
	unsigned long int result;			// used for storing system time with microsecond precision
	unsigned int rounded;				// used for storing last 4 Bytes of timestamp (last $B of result)
	struct timeval tv;					// structure variable used in gettimeofday()
	unsigned char buffer2[payload];		// buffer to store payload
	unsigned int new_rounded = 0;		// used for storing last 4B of timestamp at the time of receiving packet from client
	unsigned long avg_rtt = 0;			// used for storing avg round trip time value

	// initializing payload buffer by some random value, say 'l'
	for(int i=0; i<payload; i++)
		buffer2[i] = 'l';
	
	for (int i = 0; i < num_packets; ++i)
	{
		// ============================= FORMING PACKET ============================================
		bzero(buffer, size);
		
		// copy seq no of 2 bytes
		seq_no = htons((unsigned short int ) i);
		memcpy(buffer, &seq_no, sizeof(unsigned short int));

		// copy ttl of 1 byte
		ttl = ttl & (unsigned int) 255;
		memcpy(buffer+6, &ttl, 1);

		// copy payload of P bytes
		memcpy(buffer+7, buffer2, sizeof(buffer2));

		// print buffer if needed
		if(_printBuffer)
		{
			for(int j=0; j<size; j++)
			{
				printf("%X ",buffer[j] );
			}
			printf("\n===============================\n");
		}

		// copy timestamp of 4 bytes
		gettimeofday(&tv, NULL);
		result = tv.tv_usec + (1000000 * tv.tv_sec);
		rounded = result & (unsigned int ) MAX_UNSIGNED_INT;
		rounded = htonl(rounded);
		memcpy(buffer+2, &rounded, sizeof(unsigned int));
		// ============================= SENDING PACKET ============================================

		sendto(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr, fromlen);

		//  ================================== RECEIVING PACKET ============================================
		bzero(buffer, size);
		recvfrom(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr , &fromlen);

		// =================================== CALCULATING RTT ============================================
		//copy ttl
		int temp ;
		memcpy(&temp, buffer+6, 1);		// storing decreented ttl

		// copy timestamp of 4 bytes
		gettimeofday(&tv, NULL);
		result = tv.tv_usec + (1000000 * tv.tv_sec);
		new_rounded = result & (unsigned int ) MAX_UNSIGNED_INT;
		memcpy(&rounded, buffer+2, sizeof(unsigned int));
		rounded = ntohl(rounded);
		avg_rtt += new_rounded - rounded;
		
		if(_printRTT)
			printf("Seq_no: %u RTT: %u micro seconds\n", seq_no, new_rounded - rounded);
	}

	printf("AVG RTT: %lu\n", avg_rtt / num_packets);
	return 0;
}
