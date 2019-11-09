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
#include "THelp.h"

// Prototypes for private functions
int MainProgramLoop(int a_file_descriptor);
int SendFile(int server_file_descriptor, char* filepath, int filesize);
int ReceiveFile(int server_file_descriptor, char* filename, int filesize);

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
	int send_send_buffer;			// Send socket message this iteration?
	int connected_to_server = 0;	// default 0, -1 failed to connect, 1 connected
	int user_authorized = 0;		// default 0, 1 authorized.
	
	// User Auth
	char server_ip[BUFFER_SIZE];
	char username[BUFFER_SIZE];
	char password[BUFFER_SIZE];

	// File IO
	char tsend_filename[BUFFER_SIZE];
	int tsend_filesize = 0;
	char treceive_filename[BUFFER_SIZE];
	int treceive_filesize = 0;

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

		// TCONNECT
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
				// Save the filename for when we get the okay-to-send response
				sprintf(treceive_filename, "%s", filename);

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
					tsend_filesize = GetFilesize(tsend_filename);

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
					// The server responds that our login attempt was a success.
					sprintf(err_msg, "SERVER: Successfully signed in to TigerS!");

					user_authorized = 1;
				}
				else if (strstr(read_buffer, RES_AUTHFAILED))
				{
					// The server responds that our login attempt was a failre.
					sprintf(err_msg, "SERVER: Could not sign in to TigerS. Failed to authorize user.");

					user_authorized = 0;
				}
				else if (strstr(read_buffer, RES_UNAUTH))
				{
					// The server cannot process our command because we have not logged in yet.
					sprintf(err_msg, "SERVER: User is not authorized. Please re-run tconnect and with valid credentials.");
				}
				else if (strstr(read_buffer, RES_READY_TO_RECEIVE))
				{
					// The server is ready to receive our file. We have to send that file.
					sprintf(err_msg, "SERVER: Ready to receive.");

					// Interrupt loop to transfer data
					int success = SendFile(server_file_descriptor, tsend_filename, tsend_filesize);

					if (success == 1)
						sprintf(err_msg, "SERVER: Successfully transferred file.");
					else
						sprintf(err_msg, "SERVER: An error has occured while trying to transfer the file.");
				}
				else if (strstr(read_buffer, RES_READY_TO_SEND))
				{
					// The server is ready to send a file. We have to acknolwedge and read that file.
					sprintf(err_msg, "SERVER: Ready to send.");

					// Get the filesize specified using highly advanced scientific technology
					char* size_token = GetParam(read_buffer, 1, " \n");
					int i = 0;
					treceive_filesize = 0;
					while (size_token[i])
					{
						treceive_filesize = treceive_filesize * 10 + size_token[i] - '0';
						i++;
					}

					// Interrupt loop to receive data
					int success = ReceiveFile(server_file_descriptor, treceive_filename, treceive_filesize);

					if (success == 1)
						sprintf(err_msg, "SERVER: Successfully received file.");
					else
						sprintf(err_msg, "SERVER: An error has occured while trying to transfer the file.");

				}
				else if (strstr(read_buffer, RES_CANNOTFINDFILE))
				{
					// The server cannot send a file because it could not be found.
					sprintf(err_msg, "SERVER: cannot find file specified: %s", treceive_filename);
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
	Client used tput
	Send a file specified by the user
*/
int SendFile(int server_file_descriptor, char* filepath, int filesize)
{
	int retVal = 0;
	FILE* file = fopen(filepath, "rb");

	// Store the file in a buffer
	char* file_buffer = malloc(filesize+1);
	memset(file_buffer, 0, filesize + 1);

	char send_buffer[BUFFER_SIZE];
	
	if (file_buffer == NULL || send_buffer == NULL) 
	{
		fprintf(stderr, "Error at line %d: failed to malloc.\n", __LINE__);
	}
	else
	{
		// Copy the contents into a buffer
		fread(file_buffer, sizeof(char), filesize, file);

#ifdef TEST_BINARY_READ
		// Test the binary read
		fprintf(stderr, "Reading binary: \n");
		for (int i = 0; i < filesize; i++)
		{
			fprintf(stderr, "%u", file_buffer[i]);
		}
		fprintf(stderr, "\n\n");
#endif

		/*
		// Unused because I though the error in my binary file was because it had nulls.
		// Turned out it was just because the file was too large to send at once.
		// Hacky thing to do -> "encrypt" so there are no zeroes
		for (int i = 0; i < filesize; i++)
		{
			char val = file_buffer[i];

			uint8_t val1 = (val & 0xF0) | 0x0F;
			uint8_t val2 = (val & 0x0F) | 0xF0;
			
			send_buffer[2 * i] = val1;
			send_buffer[2 * i + 1] = val2;
		}

		*/

		// Send the data in groups of BUFFER_SIZE
		for (int i = 0; i < (filesize / BUFFER_SIZE) + 1; i++)
		{
			memset(send_buffer, 0, BUFFER_SIZE);

			// Send the next set of BUFFER_SIZE
			for (int j = 0; j < BUFFER_SIZE; j++)
			{
				// Copy the data from the file buffer to the send buffer
				if (i*BUFFER_SIZE + j < filesize)
				{
					send_buffer[j] = file_buffer[i*BUFFER_SIZE + j];
				}
			}
			send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);
		}

		// Send it over
		// send(server_file_descriptor, (void *)send_buffer, 2* (filesize + 1), 0);
		
		// Read response
		char read_buffer[BUFFER_SIZE];
		if (read(server_file_descriptor, read_buffer, BUFFER_SIZE))
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


/*
	Client used tget
	Receive a file
*/
int ReceiveFile(int server_file_descriptor, char* filename, int filesize)
{
	int retVal = 0;

	// Response (success or fail)
	char send_buffer[BUFFER_SIZE];

	// Determine the filepath 
	char filepath[BUFFER_SIZE];
	sprintf(filepath, "%s%s", SERVER_FILE_DIR, filename);
	FILE* file = fopen(filepath, "wb");

	printf("Receiving file of size %d and saving as: %s\n", filesize, filepath);

	// Store the incoming contents in a buffer
	char* file_buffer = malloc(filesize + 1);
	memset(file_buffer, 0, filesize + 1);
	if (file_buffer)
	{
		if (read(server_file_descriptor, file_buffer, filesize + 1))
		{
			file_buffer[filesize] = 0;
			fwrite(file_buffer, sizeof(char), filesize, file);
			fclose(file);

			printf("Success! File %s saved to %s!\n", filename, filepath);

			sprintf(send_buffer, "%s", RES_RECEIVE_SUCCESS);
			retVal = 1;

			// Send the response
			send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);
		}
		else
		{
			fprintf(stderr, "Error at line %d: failed to receice data.\n", __LINE__);

			sprintf(send_buffer, "%s", RES_RECEIVE_FAILURE);
			retVal = -1;

			// Send the response
			send(server_file_descriptor, send_buffer, strlen(send_buffer), 0);
		}
	}
	else
	{
		fprintf(stderr, "Error at line %d: failed to malloc.\n", __LINE__);
	}

	// Cleanup
	if (file_buffer)
	{
		free(file_buffer);
		file_buffer = NULL;
	}

	return retVal;
}
