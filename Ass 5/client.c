#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/stat.h>

#define DEFAULT_PACKET_SIZE 516
#define DEFAULT_FIELNAME_LENGTH 20
#define DEFAULT_CMD_LENGTH 50
#define DEFAULT_DATA_SIZE 512
#define DEFAULT_SEND_TIMEOUT 5
#define DEFAULT_RECV_TIMEOUT 5
#define MAX_RRQ 5
#define MAX_WRQ 5

#define PROMPT "\n_tftp_>"

#define debug_print 0
#define print_buffer_sent 0
#define print_buffer_recv 0
#define print_transmission_stat 1

void del(int count)
{
	for (int i = 0; i < count; ++i){
		fprintf(stderr, "\b");
	}
}
void *show_trans_speed( void *ptr)
{
	size_t b_write_total = *((int*) ptr);
	size_t b_temp = 0;
	b_temp = *((int *) ptr);
	while(true){
		del(50);
		fprintf(stderr, "Transmission Speed: %.2f KB/sec  ", (float)  (*((unsigned long int *) ptr) - b_temp)/1024);
		b_temp = *((int *) ptr);
		sleep(1);
	}
	
}
void print_buffer(unsigned char *buffer, int length)
{
	fprintf(stderr, "BUFFER: ");
	for (int i = 0; i < length; ++i)
	{
		fprintf(stderr, "%x",buffer[i] );
	}
	fprintf(stderr, "\n" );
}
unsigned long int gimme_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long int result = tv.tv_usec + (1000000 * tv.tv_sec);
	return result;
}
int main(int argc, char const *argv[])
{
	if(argc != 2){
		fprintf(stderr, "Usage: %s [server ip] \n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char server_ip[15];																// server ip
	sscanf(argv[1], "%s", server_ip);
	int sockfd;
	int portno = 69;
	struct sockaddr_in serv_addr;
	socklen_t fromlen;
	struct timeval tv;
	unsigned char buffer[DEFAULT_PACKET_SIZE], temp_buf[DEFAULT_PACKET_SIZE];
	unsigned char data[DEFAULT_DATA_SIZE];
	unsigned char opcode[2];
	unsigned char filename[DEFAULT_FIELNAME_LENGTH];
	unsigned char mode[] = "octet";
	unsigned char command[DEFAULT_CMD_LENGTH];
	size_t b_sent, b_recv = 600, b_write, b_write_total = 0;
	// socket call
	sockfd = socket(AF_INET , SOCK_DGRAM , 0);
	if(sockfd == -1)
	{
		fprintf(stderr, "Failed in creating socket. \n");
		exit(0);
	}
	if(debug_print)
		fprintf(stderr, "Socket Created \n" );


	// assigning values 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);

	fromlen = sizeof(serv_addr);

	tv.tv_sec = DEFAULT_RECV_TIMEOUT;
	tv.tv_usec = 0;
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void*) &tv, sizeof(tv)) != 0){
		fprintf(stderr, "SOCKET receive timeout setting failed. Please initiate again. ERRNO: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	tv.tv_sec = DEFAULT_SEND_TIMEOUT;
	tv.tv_usec = 0;
	if( setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const void*) &tv, sizeof(tv)) != 0){
		fprintf(stderr, "SOCKET send timeout setting failed. Please initiate again. ERRNO: %d\n", errno);
		exit(EXIT_FAILURE);
	}


	while(true){
		fprintf(stderr, "%s", PROMPT);
		memset(command, 0, sizeof(command));
		fgets(command, sizeof(command), stdin);

		if( strncmp(command, "get", 3) == 0){
			// download
			
			int rrq_len;
			int rrq_count = 0;
			unsigned short block_no, prev_block_no;
			int ack = 0;
			int fd;
			bool first_data = true;
			bool rrq_served = false;
			bool print_transmission_result = true;
			bool break_condition = true;
			unsigned long int time1 = gimme_time();
			pthread_t tid;
			size_t b_sent, b_recv = 600, b_write, b_write_total = 0;
			
			memset(buffer, 0, DEFAULT_PACKET_SIZE);								// set buffer as 0
			
			opcode[0] = 0x00;													// set and copy opcode
			opcode[1] = 0x01;
			memcpy(buffer, &opcode, sizeof(opcode));
			
			memset(filename, 0, sizeof(filename));
			sscanf(command+3, "%s", filename);									// set and copy filename 
			memcpy(buffer+2, filename, strlen(filename));
			
			memcpy(buffer+2+strlen(filename)+1, mode, strlen(mode));
			
			rrq_len = 2+strlen(filename)+1+strlen(mode)+1;						// send RRQ
			b_sent = sendto(sockfd , buffer , rrq_len , 0 , (struct sockaddr *) &serv_addr, fromlen);
			rrq_count++;

			if(debug_print){
				fprintf(stderr, "%ld Bytes of RRQ sent to Server: '%s' for File: '%s'\n", b_sent, server_ip, filename);
			}
			if(print_buffer_sent)
				print_buffer(buffer, rrq_len);
			
			while(break_condition){													// while server sends data, keep receiving
				memcpy(temp_buf, buffer, sizeof(buffer));
				memset(buffer, 0, sizeof(buffer));								// clear buffer
				while(true && rrq_count<MAX_RRQ){
					b_recv = recvfrom(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr , &fromlen);// receive data from server
					if(b_recv == -1){
						fprintf(stderr, "RECV TIMEOUT: Sending RRQ again\n");
						b_sent = sendto(sockfd , temp_buf , rrq_len , 0 , (struct sockaddr *) &serv_addr, fromlen);
						if(debug_print){
							fprintf(stderr, "%ld Bytes of RRQ sent to Server: '%s' for File: '%s'\n", b_sent, server_ip, filename);
						}
						if(print_buffer_sent)
							print_buffer(temp_buf, rrq_len);
						rrq_count++;
					}
					else
						break;
				}
				if(b_recv < DEFAULT_PACKET_SIZE){
					break_condition = false;
					fprintf(stderr, "here\n" );
				}
				if(debug_print){
					fprintf(stderr, "%ld Bytes Data received from Server.\n", b_recv);
				}
				if(print_buffer_recv)
					print_buffer(buffer, b_recv);

				if(buffer[1] == 3){												// enter in if packet received is a data packet
					if(first_data){												// if it is the first data packet then create file to be received
						fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0777);
						int err_no = errno;
						if(fd == -1){											// break if any error is present
							fprintf(stderr, "Failed to create file %s\n", filename);
							if(errno == EEXIST)
								fprintf(stderr, "File %s already present in current directory.\n", filename);
							print_transmission_result = false;
							break;
						}
						first_data = false;
						fprintf(stderr, "Connected to server: %s\n", server_ip);
						if(print_transmission_stat){
							pthread_create(&tid, NULL, show_trans_speed, &b_write_total);
						}
					}
					prev_block_no = block_no;
					memcpy(&block_no, buffer+2, sizeof(block_no));			// copy block no
					if(prev_block_no == block_no)
						continue;
					memset(data, 0, sizeof(data));							// clear data
					memcpy(data, buffer+4, b_recv-4);						// copy data
					b_write = write(fd, data, b_recv-4);					// write data
					b_write_total += b_write;								// store total data written into file

					memset(buffer, 0, sizeof(buffer));						// clear buffer
					opcode[1] = 0x04;										// set opcode
					memcpy(buffer, opcode, sizeof(opcode));					// copy opcode
					memcpy(buffer+2, &block_no, sizeof(block_no));			// copy block no
					b_sent = sendto(sockfd , buffer , 4 , 0 , (struct sockaddr *) &serv_addr, fromlen);// send ACK
					if(debug_print){
						fprintf(stderr, "%ld bytes of ACK[count: %d] for block no: %u sent\n", b_sent, ack, block_no );
						fprintf(stderr, "%ld Bytes of data received from server in total\n", b_write_total);
					}
					if(print_buffer_sent)
						print_buffer(buffer, b_sent);
					ack++;
					
				}
				else if(buffer[1] == 5){
					fprintf(stderr, "Error from server: %s\n", server_ip);
					print_transmission_result = false;
					break;
				}	
			}
			if(print_transmission_result){		
				close(fd);
				fprintf(stderr, "\n%ld Bytes (%f MB) data written into file %s\n", b_write_total, (float) b_write_total/(1024*1024), filename);
				fprintf(stderr, "  Total Packet Received: %d\n", ack );
				fprintf(stderr, "Avg. Transmission Speed: %f KB/sec\n", (float)( (b_write_total/1024)/((gimme_time() - time1)/1000000)));
				pthread_cancel(tid);
			}
		
		}
		else if( strncmp(command, "put", 3) == 0){
			// upload
			int wrq_len;
			int wrq_count = 0;
			unsigned short block_no = 0, prev_block_no;
			unsigned int temp_block_no;
			int ack = 0;
			unsigned long int time1 = gimme_time();
			pthread_t tid;
			bool break_condition = true;
			bool wrq_served = false;
			bool print_transmission_result = true;
			size_t b_sent, b_recv = 600, b_read, b_read_total = 0;

			memset(buffer, 0, sizeof(buffer));
			opcode[0] = 0x00;
			opcode[1] = 0x02;
			memcpy(buffer, opcode, sizeof(opcode));

			memset(filename, 0, sizeof(filename));
			sscanf(command+3, "%s", filename);
			memcpy(buffer+2, filename, strlen(filename));

			memcpy(buffer+2+strlen(filename)+1, mode, strlen(mode));
			wrq_len = 2+strlen(filename)+1+strlen(mode)+1;
			int fd = open(filename, O_RDONLY, 0777);
			if(fd == -1){
				fprintf(stderr, "No such file is present in current directory: %s\n", filename );
				continue;
			}
			if( lseek(fd, 0, SEEK_SET) != 0){
				fprintf(stderr, "Seek failure\n" );
				continue;
			}
			if(print_transmission_stat){
				pthread_create(&tid, NULL, show_trans_speed, &b_read_total);
			}

			if(debug_print)
				fprintf(stderr, "File %s opened for reading.\n", filename);
			
			b_sent = sendto(sockfd , buffer , wrq_len , 0 , (struct sockaddr *) &serv_addr, fromlen);
			if(debug_print)
				fprintf(stderr, "%ld bytes of WRQ sent to server '%s' for file '%s'\n", b_sent, server_ip, filename );
			
			memcpy(temp_buf, buffer, sizeof(buffer));
			while(break_condition){
				// getchar();
				memset(buffer, 0, sizeof(buffer));
				b_recv = recvfrom(sockfd , buffer , sizeof(buffer) , 0 , (struct sockaddr *) &serv_addr , &fromlen);
				if(b_recv == -1){
					fprintf(stderr, "RECV TIMEOUT\n");
					if(wrq_count<MAX_WRQ && !wrq_served){
						fprintf(stderr, "Sending WRRQ again\n" );
						b_sent = sendto(sockfd , temp_buf , wrq_len , 0 , (struct sockaddr *) &serv_addr, fromlen);
						if(debug_print){
							fprintf(stderr, "%ld Bytes of WRQ sent to Server: '%s' for File: '%s'\n", b_sent, server_ip, filename);
						}
						if(print_buffer_sent)
							print_buffer(temp_buf, wrq_len);
						wrq_count++;
					}
					if(wrq_served){
						// send prev data frame
						fprintf(stderr, "Sending Previous Data frame\n" );
						b_sent = sendto(sockfd , temp_buf , b_read+4 , 0 , (struct sockaddr *) &serv_addr, fromlen);
						if(debug_print)
							fprintf(stderr, "%ld bytes of data sent to server for block no %d\n", b_sent, block_no);
						if(print_buffer_sent)
							print_buffer(buffer, b_sent);
					}
				}
				wrq_served = true;
				if(buffer[1] == 4){
					memcpy(&block_no, buffer+2, sizeof(2));
					if(debug_print)
						fprintf(stderr, "%ld Bytes of ACK[count: %d] received from server for block no %d \n", b_recv, ack, block_no);
					if(print_buffer_recv)
						print_buffer(buffer, 4);
					prev_block_no = block_no;
					if(block_no == 65535)
						block_no = 0;
					else if(block_no >= 65280){
						temp_block_no = block_no;
						temp_block_no += 256;
						block_no = temp_block_no & 0xffff;
						temp_block_no = temp_block_no >> 16;
						block_no += temp_block_no & 0xffff;
					}
					else
						block_no += 256;
					ack++;
					memset(data, 0, sizeof(data));

					b_read = read(fd, data, DEFAULT_DATA_SIZE);
					if(debug_print)
						fprintf(stderr, "%ld bytes read from file %s\n", b_read, filename );
					if(b_read == -1){
						fprintf(stderr, "Unable to read from file %s\n", filename);
							break;
					}
					if(b_read < DEFAULT_DATA_SIZE)
						break_condition = false;
					// fprintf(stderr, "Break Condition\n", );
					memset(buffer, 0, sizeof(buffer));
					opcode[0] = 0x00;
					opcode[1] = 0x03;
					memcpy(buffer, opcode, sizeof(data));
					memcpy(buffer+2, &block_no, sizeof(block_no));
					memcpy(buffer+4, data, b_read);
					memcpy(temp_buf, buffer, b_read+4);
					b_sent = sendto(sockfd , buffer , b_read+4 , 0 , (struct sockaddr *) &serv_addr, fromlen);
					b_read_total += b_sent - 4;
					if(debug_print)
						fprintf(stderr, "%ld bytes of data sent to server for block no %d\n", b_sent, block_no);
					if(debug_print)
						fprintf(stderr, "%ld bytes in total sent to server\n", b_read_total);
					if(print_buffer_sent)
						print_buffer(buffer, b_sent);
				}
				else if(buffer[1] == 5){
					fprintf(stderr, "Error from server: %s\n", server_ip);
					print_transmission_result = false;
					break;
				}
			}
			if(print_transmission_result){		
				close(fd);
				fprintf(stderr, "\n%ld Bytes (%f MB) data sent to server %s\n", b_read_total, (float) b_read_total/(1024*1024), server_ip);
				fprintf(stderr, "  Total Packet sent: %d\n", ack );
				fprintf(stderr, "Avg. Transmission Speed: %f KB/sec\n", (float)( (b_read_total/1024)/((float)(gimme_time() - time1)/1000000)));
				if(print_transmission_stat)
					pthread_cancel(tid);
			}

		}
		else if( strncmp(command, "quit", 4) == 0 || strncmp(command, "exit", 4) == 0){
			if(debug_print)
				fprintf(stderr, "Exiting...\n" );
			exit(EXIT_SUCCESS);
		}
		else
			fprintf(stderr, "%s: Invalid Command\n", argv[0]);
	}
	
	return 0;
} 