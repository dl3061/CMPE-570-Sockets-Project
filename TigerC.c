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
int CheckIfFileExists(char* filename);
int SendFile(int a_file_descripttor, char* filepath, int filesize);

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
	// Client sends first, then reads
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

	char tsend_filename[BUFFER_SIZE];
	int tsend_filesize = 0;

	int send_send_buffer;

	while (keep_program_alive)
	{
		// Flag to determine whether or not we're sending and expecting a response this
		//		iteration, depending on the user's input.
		send_send_buffer = 0;

		// Clear and update display
		printf(T_CLEARSCREEN);
		
		// Connection status
		if (connected_to_server == (-1))
			printf("Failed to connect to TigerS at %s.\n", server_ip);
		else if (connected_to_server == (1))
			printf("Connected to TigerS at %s.\n", server_ip);
		else
			printf("Not connected to server.\n");

		// Login / authorization status
		if (!user_authorized)
			printf("Currently not signed in.\n");
		else
			printf("Signed in as %s!\n", username);

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

		// TCONNECTr
		if (strstr(input_buffer, CMD_TCONNECT) == input_buffer)
		{
			// tconnect
			if (verbose)
				printf("Got a tconnect command!\n");

			// Get the IP, username and password
			char* ip_token = GetParam(input_buffer, 1, " \n");
			char* username_token = GetParam(input_buffer, 2, " \n");
			char* password_token = GetParam(input_buffer, 3, " \n");

			if (ip_token == NULL || username_token == NULL || password_token == NULL)
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

						// Log in
						sprintf(send_buffer, "%s %s %s", CMD_TCONNECT, username, password);
						send_send_buffer = 1;
					}
				}
			}
		}
		
		// TGET
		else if (strstr(input_buffer, CMD_TGET))
		{
			if (verbose)
				printf("Got a tget command!\n");

			char* filename = GetParam(input_buffer, 1, " \n");
			if (filename == NULL)
			{
				// Invalid command 
				sprintf(err_msg, "Invalid param. Expected: tget <File Name>");
			}
			else
			{
				// Send the request to download the file
				sprintf(send_buffer, "%s %s", CMD_TGET, filename);

				send_send_buffer = 1;
			}
		}
		
		// TPUT
		else if (strstr(input_buffer, CMD_TPUT))
		{
			if (verbose)
				printf("Got a tput command!\n");

			char* filename = GetParam(input_buffer, 1, " \n");
			if (filename == NULL)
			{
				// Invalid command 
				sprintf(err_msg, "Invalid param. Expected: tget <File Name>");
			}
			else
			{
				// Save the filename for when we get the okay-to-send response
				sprintf(tsend_filename, "%s", filename);

				// Verify if the file exists
				if (CheckIfFileExists(tsend_filename))
				{
					// Get the filesize
					FILE* tsend_file = fopen(tsend_filename, "rb");
					fseek(tsend_file, 0L, SEEK_END);
					tsend_filesize = ftell(tsend_file);
					fseek(tsend_file, 0L, SEEK_SET);
					fclose(tsend_file);

					// Send the request to download the file
					sprintf(send_buffer, "%s %s %d", CMD_TPUT, tsend_filename, tsend_filesize);

					send_send_buffer = 1;
				}
				else
				{
					sprintf(err_msg, "Cannot find/read file %s.", tsend_filename);
				}
			}
		}
		else if (strstr(input_buffer, CMD_END))
		{
			if (verbose)
				printf("Got an end command!\n");

			sprintf(send_buffer, "%s", CMD_END);

			if ((connected_to_server == 0) || (user_authorized == 0))
				keep_program_alive = 0;

			send_send_buffer = 1;
		}
		else
		{
			printf("Unrecognized input %s.\n", input_buffer);
			sprintf(err_msg, "Unrecongized input: %s.", input_buffer);
		}


		// Send outgoing command and read the response if applicable
		if (send_send_buffer && (connected_to_server == 1))
		{
			// Send the outgoing command from send_buffer
			send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);

			// Read response data and save it to the buffer
			memset(read_buffer, 0, BUFFER_SIZE);
			if (read(server_file_descriptor, read_buffer, BUFFER_SIZE))
			{
				// Print the data in
				if (verbose)
					printf("Received from server: %s \n", read_buffer);
				fprintf(stderr, "Received from server: %s \n", read_buffer);

				if (strstr(read_buffer, RES_AUTH))
				{
					user_authorized = 1;
					sprintf(err_msg, "SERVER: Successfully signed in to TigerS!");
				}
				else if (strstr(read_buffer, RES_AUTHFAILED))
				{
					user_authorized = 0;
					sprintf(err_msg, "SERVER: Could not sign in to TigerS. Failed to authorize user.");
				}
				else if (strstr(read_buffer, RES_UNAUTH))
				{
					sprintf(err_msg, "SERVER: User is not authorized. Please re-run tconnect and with valid credentials.");
				}
				else if (strstr(read_buffer, RES_READY_TO_RECEIVE))
				{
					sprintf(err_msg, "SERVER: Ready to receive.");

					// Interrupt loop to transfer data
					int success = SendFile(server_file_descriptor, tsend_filename, tsend_filesize);

					if (success == 1)
						sprintf(err_msg, "SERVER: Successfully transferred file.");
					else
						sprintf(err_msg, "SERVER: An error has occured while trying to transfer the file.");
				}
				else if (strstr(read_buffer, RES_ENDCLIENT) || strstr(read_buffer, RES_KILLCIENT))
				{
					user_authorized = 0;
					keep_program_alive = 0;
				}
			}
		}
	}

	return 0;
}


/*
	Checks if a file exists (and if it has read permission)
*/
int CheckIfFileExists(char* filename)
{
	if (access(filename, R_OK) != -1) 
	{
		return 1;
	}
	else 
	{
		return 0;
	}
}


/*
	
*/
int SendFile(int a_file_descripttor, char* filepath, int filesize)
{
	int retVal = 0;
	FILE* file = fopen(filepath, "rb");

	// Store the file in a buffer
	char* file_buffer = malloc(filesize+1);
	memset(file_buffer, 0, filesize + 1);
	if (file_buffer == NULL)
	{
		fprintf(stderr, "Error at line %d: failed to malloc.\n", __LINE__);
	}
	else
	{
		// Copy the contents into a buffer
		fread(file_buffer, sizeof(char), filesize, file);

		for (int i = 0; i < filesize; i++)
		{
			fprintf(stderr, "%u", file_buffer[i]);
		}
		fprintf(stderr, "\n\n");

		// Send it over
		send(a_file_descripttor, file_buffer, strlen(file_buffer), 0);
		
		// Read response
		char read_buffer[BUFFER_SIZE];
		if (read(a_file_descripttor, read_buffer, BUFFER_SIZE))
		{
			if (strstr(read_buffer, RES_RECEIVE_SUCCESS))
			{
				retVal = 1;
			}
			else
			{
				fprintf(stderr, "SERVER: %s", read_buffer);
				retVal = -1;
			}
		}
	}

	// Cleanup
	if (file_buffer)
	{
		free(file_buffer);
		file_buffer = NULL;
	}

	fclose(file);

	return retVal;
}


