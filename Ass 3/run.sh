#1/bin/bash
gcc -Wall -o server Server.c
gcc -Wall -o client Client.c

echo "Complitaion Completed"
echo "Usage:"
echo "		1. server  <server_port>"
echo "		2. client  <server_IP>  <server_port>  <P>  <TTL>  <Num_of_Packets"
