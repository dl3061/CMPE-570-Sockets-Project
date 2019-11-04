/*
	TigerC.c
	David Lin (dl3061@rit.edu)
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "Tiger.h"
#include "TParam.h"

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

	// Set verbose to true for debugging
	verbose = 1;

	// Main
	MainProgramLoop(socket_fd);
}


int MainProgramLoop(int server_file_descriptor)
{
	// Server reads first, then sends
	int keep_program_alive = 1;
	char read_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];
	char input_buffer[BUFFER_SIZE];
	char err_msg[BUFFER_SIZE];

	// Variables
	int connected_to_server = 0;	// default 0, -1 failed to connect, 1 connected
	int user_authorized = 0;		// default 0, 1 authorized.
	char server_ip[BUFFER_SIZE];
	char username[BUFFER_SIZE];
	char password[BUFFER_SIZE];
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
		printf("\n");
		
		// Print status (if any)
		printf("%s\n", err_msg);
		printf("\n");
		memset(err_msg, 0, BUFFER_SIZE);

		// Get user input
		memset(input_buffer, 0, BUFFER_SIZE);
		fgets(input_buffer, BUFFER_SIZE, stdin);
		if (verbose)
			printf("Received user cmd: %s\n", input_buffer);

		// Parse user input

		// TCONNECT
		if (strstr(input_buffer, CMD_TCONNECT))
		{
			// tconnect
			if (verbose)
				printf("Got a tconnect command!\n");

			// Get the IP, username and password
			char* tconnect_token = GetParam(input_buffer, 0, " ");
			char* ip_token = GetParam(input_buffer, 1, " ");
			char* username_token = GetParam(input_buffer, 2, " ");
			char* password_token = GetParam(input_buffer, 3, " ");

			if (tconnect_token == NULL || ip_token == NULL || username_token == NULL || password_token == NULL)
			{
				// Invalid command
				sprintf(err_msg, "Invalid params. Expected: tconnect <TigerS IP Address> <User> <Password>");
			}
			else
			{
				// Save params in their respective variables
				sprintf(server_ip, "%s", ip_token);
				sprintf(username, "%s", username_token);
				sprintf(password, "%s", password_token);

				// Address(?)
				struct sockaddr_in serv_addr;
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(PORT);

				// Convert IPv4 and IPv6 addresses from text to binary form 
				if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
				{
					fprintf(stderr, "Error at line %d: Invalid address %s. Address not supported. \n",
						__LINE__, server_ip);

					// Set failure flag
					connected_to_server = (-1);
				}
				else
				{
					if (connect(server_file_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
					{
						fprintf(stderr, "Error at line %d: Connection to IP %s failed.\n",
							__LINE__, server_ip);

						// Set failure flag
						connected_to_server = (-1);
					}
					else
					{
						// Successfully connected
						connected_to_server = 1;
					}
				}
			}
		}
		// TGET
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


		// Send outgoing command and read the response if applicable
		if (send_send_buffer)
		{
			// Send the outgoing command from send_buffer
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
	}

	// Send outgoing command to end the program
	sprintf(send_buffer, "Testing: %s", CMD_END);
	send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);

	return 0;
}