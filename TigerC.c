#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "Tiger.h"

int MainProgramLoop(int a_file_descriptor);

int verbose = 0;

int main()
{
	// Initialize
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		// AF_INET -> IPV4 protocol
		// SOCK_STREAM -> TCP
		// Protocol -> 0 (Internet Protocol
	if (socket_fd  < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	verbose = 1;
	MainProgramLoop();
}


int MainProgramLoop(int server_file_descriptor)
{
	// Server reads first, then sends
	int keep_program_alive = 1;
	char read_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];
	char input_buffer[BUFFER_SIZE];

	// Variables
	int connected_to_server = 0;	// default 0, -1 failed to connect, 1 connected
	int user_authorized = 0;		// default 0, 1 authorized.
	char server_ip[BUFFER_SIZE];
	char username[BUFFER_SIZE];
	int send_send_buffer;

	while (keep_program_alive)
	{
		// Flag to determine whether or not we're sending and expecting a response this
		//		iteration, depending on the user's input.
		send_send_buffer = 0;

		// Clear and update display
		printf(T_CLEARSCREEN);
		
		// Connection status
		if (connected_to_server == 0)
			printf("Not connected to server.\n");
		else if (connected_to_server == (-1))
			printf("Failed to connect to TigerS at %s.\n", server_ip);
		else
			printf("Failed to connect to TigerS at %s.\n", server_ip);

		// Login / authorization status
		if (!user_authorized)
			printf("\tCurrently not signed in.\n");
		else
			printf("\tSigned in as %s!\n", username);

		// Print commands
		printf("Supported commands:\n");
		printf("\t%s <TigerS IP Address> <User> <Password>\n", CMD_TCONNECT);
		printf("\t%s <File Name>\n", CMD_TGET);
		printf("\t%s <File Name>\n", CMD_TPUT);
		printf("\t%s\n", CMD_END);
		printf("\n\n");

		// Get user input
		memset(input_buffer, 0, BUFFER_SIZE);
		fgets(input_buffer, BUFFER_SIZE, stdin);
		if (verbose)
			printf("Received user cmd: %s\n", input_buffer);

		// Parse user input
		if (strstr(input_buffer, CMD_TCONNECT))
		{
			// tconnect
			if (verbose)
				printf("Got a tconnect command!\n");

			// Address(?)
			struct sockaddr_in serv_addr;
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(PORT);

			// Convert IPv4 and IPv6 addresses from text to binary form 
			if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
			{
				fprintf(stderr, "Error at line %d: Invalid address %s. Address not supported. \n",
					__LINE__, server_ip);

				// Set failure flag
				connected_to_server = (-1);
			}
			else
			{
				if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
				{
					fprintf(stderr, "Error at line %d: Connection to IP %s failed.\n",
						__LINE__, server_ip);

					// Set failure flag
					connected_to_server = (-1);
				}
				else
				{

				}
			}

		}
		else if (strstr(input_buffer, CMD_TGET))
		{
			if (verbose)
				printf("Got a tget command!\n");

			//@todo 

			send_send_buffer = 1;
		}
		else if (strstr(input_buffer, CMD_TPUT))
		{
			if (verbose)
				printf("Got a tput command!\n");

			//@todo 

			send_send_buffer = 1;
		}
		else if (strstr(input_buffer, CMD_END))
		{
			if (verbose)
				printf("Got an end command!\n");

			//@todo 

			send_send_buffer = 1;
		}
		else
		{
			printf("Unrecognized input.");
		}



		// Send outgoing command
		sprintf(send_buffer, "%s", input_buffer);
		send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);

		// Read response data and save it to the buffer
		memset(read_buffer, 0, BUFFER_SIZE);
		if (read(server_file_descriptor, read_buffer, BUFFER_SIZE))
		{
			// Print the data in
			if (verbose)
				printf("Received from server: %s \n", read_buffer);

			if (strstr(read_buffer, RES_AUTH))
			{
				user_authorized = 1;
			}
			else if (strstr(read_buffer, RES_ENDCLIENT) || strstr(read_buffer, RES_KILLCIENT))
			{
				user_authorized = 0;
				keep_program_alive = 0;
			}
		}
	}

	// Send outgoing command
	sprintf(send_buffer, "Testing: %s", CMD_END);
	send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);

	return 0;
}