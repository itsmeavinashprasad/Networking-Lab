#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<sys/time.h>

#define MAX_UNSIGNED_INT 0xFFFFFFFF //4294967295
#define _printRTT 0
#define _printAvgRTT 0

unsigned int gimme_time()
{
	// this func returns last 4 bits of system time
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long int result = tv.tv_usec + (1000000 * tv.tv_sec);
	unsigned int rounded = result & (unsigned int ) MAX_UNSIGNED_INT;
	return rounded;
}

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
	fprintf(stderr, "Socket created\n");

	// assigning values 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);

	fromlen = sizeof(serv_addr);
	
	int size = 7 + payload;				// size of the packet
	unsigned char buffer[size];			// buffer used for sending and receiving
	unsigned short int seq_no;			// used for storing seq no
	unsigned int rounded;				// used for storing last 4 Bytes of timestamp (last $B of result)
	unsigned char buffer2[payload];		// buffer to store payload
	unsigned int new_rounded = 0;		// used for storing last 4B of timestamp at the time of receiving packet from client
	unsigned long avg_rtt = 0;			// used for storing avg round trip time value
	unsigned long cum_rtt = 0;			// used for storing cum round trip time value
	int temp_ttl = 0;

	// initializing payload buffer by some random value, say 'l'
	for(int i=0; i<payload; i++)
		buffer2[i] = 'l';
	
	FILE *fp = fopen("data_localhost2.csv","a");				// opening file
	if(fp == NULL){
		fprintf(stderr, "Failed in Opening file\n" );
		exit(1);
	}
	fprintf(stderr, "File Opened\n" );
	fprintf(fp, "%d, %d, ", ttl, payload);					// putting some info	
	
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
		
		cum_rtt = 0;
		temp_ttl = ttl;
		
		while(temp_ttl>0)
		{
			// copy timestamp of 4 bytes
			rounded = htonl(gimme_time());
			memcpy(buffer+2, &rounded, 4);
			
			// ============================= SENDING PACKET ============================================
			sendto(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	
			//  ================================== RECEIVING PACKET ============================================
			bzero(buffer, size);
			recvfrom(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr , &fromlen);
			fprintf(stderr, "Packet Received\n" );
			for(int l=0; l<11;l++)
				fprintf(stderr, "%x ",buffer[l] );
			fprintf(stderr, "\n" );
			// =================================== CALCULATING RTT ============================================
			// copy timestamp of 4 bytes
			memcpy(&new_rounded, buffer+2, 4);
			new_rounded = ntohl(new_rounded);
			cum_rtt += new_rounded - rounded;			
			
			//copy ttl
			memcpy(&temp_ttl, buffer+6, 1);
			temp_ttl--;
			fprintf(stderr, "TTL: %d\n", temp_ttl);
			memcpy(buffer+6, &temp_ttl, 1);
			// getchar();
		}
		
		if(_printRTT)
			printf("Seq_no: %u RTT: %u micro seconds\n", seq_no, new_rounded - rounded);
		
		avg_rtt += cum_rtt;
	}
	if(_printAvgRTT)
		printf("AVG RTT: %lu\n", avg_rtt / num_packets);
	
	fprintf(fp,"%lu\n", avg_rtt);
	fclose(fp);
	return 0;
}