/*
	TigerC.c
	Main program for the Tiger client.

	Author:
	David Lin (dl3061@rit.edu)

	Additional Contributions:
	Jason Ramsden
	Austin Brogan
	Susan Margevich
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
int RequestPort(int server_file_descriptor);

#define LOCK_FILE	"client_lock.txt"
void LockClient(void);
void UnlockClient(void);

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

	// Bid farewell.
	printf("Goodbye.\n");
}


int MainProgramLoop(int server_file_descriptor)
{
	// Keeping track of server file descriptors
	int active_fd = server_file_descriptor;	// Can change after getting a port
	int orig_socket_fd = server_file_descriptor;

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
		memset(send_buffer, 0, BUFFER_SIZE);

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
		printf("\t%s <File Name On Server> as <File Name>\n", CMD_TGET);
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
				// First, try to connect to the server and get a valid port.
				if (connected_to_server == 0)
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
						if (connect(orig_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
						{
							fprintf(stderr, "Error at line %d: Connection to IP %s failed.\n",
								__LINE__, server_ip);

							// Set failure flag
							connected_to_server = (-1);
						}
						else
						{
							if (verbose)
								printf("Connected to the server! Requesting port....\n");

							// Successfully connected to port program.
							// Request a port.
							int port = RequestPort(active_fd);

							if (verbose)
								printf("Got port: %d!\n", port);

							int new_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

							// Try to connect
							// Address(?)
							struct sockaddr_in serv_addr_port;
							serv_addr_port.sin_family = AF_INET;
							serv_addr_port.sin_port = htons(port);

							// Convert IPv4 and IPv6 addresses from text to binary form 
							if (inet_pton(AF_INET, server_ip, &serv_addr_port.sin_addr) <= 0)
							{
								fprintf(stderr, "Error at line %d: Invalid address %s. Address not supported. \n",
									__LINE__, server_ip);

								// Set failure flag
								connected_to_server = (-1);
							}
							else
							{
								if (verbose)
									printf("Trying to reconnect to server at port %d...\n", port);

								if (connect(new_socket_fd, (struct sockaddr *)&serv_addr_port, sizeof(serv_addr_port)) < 0)
								{
									fprintf(stderr, "Error at line %d: Connection to IP %s at port %d failed.\n",
										__LINE__, server_ip, port);

									// Set failure flag
									connected_to_server = (-1);
								}

								connected_to_server = 1;
								active_fd = new_socket_fd;

								if (verbose)
									printf("Successfully connected to server at port %d!\n", port);
							}
						}
					}
				}
			
				// Once connected, try to authenticate. If already connected previous, just re-authenticate.
				if (connected_to_server == 1)
				{
					if (verbose)
						printf("Trying to authenticate...\n");

					// Log in
					sprintf(send_buffer, "%s %s %s", CMD_TCONNECT, username, password);
					send_send_buffer = 1;
				}
			}
		}
		
		// TGET
		else if (strstr(input_buffer, CMD_TGET))
		{
			if (verbose)
				printf("Got a tget command!\n");

			// Get the filename
			char* filename = GetParam(input_buffer, 1, " \n");

			// Check if the user specified -as override-name
			char* as_override = GetParam(input_buffer, 2, " \n");
			char* override_filename = GetParam(input_buffer, 3, " \n");
			int bOverride = (as_override != NULL) && strstr(as_override, CMD_RENAME_DOWNLOADED_FILE);

			if (filename == NULL)
			{
				// Invalid command 
				sprintf(err_msg, "Invalid param. Expected: tget <File Name>");
			}
			else if (bOverride && override_filename == NULL)
			{
				// Invalid command 
				sprintf(err_msg, "Invalid param. Expected: tget <File Name On Server> as <File Name>");
			}
			else
			{
				// Save the filename for when we get the okay-to-send response
				// sprintf(treceive_filename, "%s", filename);

				// Check if the user specified "as other-name'
				memset(treceive_filename, 0, BUFFER_SIZE);
				if (bOverride)
				{
					sprintf(treceive_filename, "%s", override_filename);
				}
				else
				{
					sprintf(treceive_filename, "%s", filename);
				}

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
		// LockClient();
		if (send_send_buffer && (connected_to_server == 1))
		{
			// Send the outgoing command from send_buffer
			send(active_fd, send_buffer, BUFFER_SIZE, 0);

			// Read response data and save it to the buffer
			memset(read_buffer, 0, BUFFER_SIZE);
			if (read(active_fd, read_buffer, BUFFER_SIZE))
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
					int success = SendFile(active_fd, tsend_filename, tsend_filesize);

					if (success == 1)
						sprintf(err_msg, "SERVER: Successfully transferred file %s of size %d.", tsend_filename, tsend_filesize);
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
					int success = ReceiveFile(active_fd, treceive_filename, treceive_filesize);

					if (success == 1)
						sprintf(err_msg, "SERVER: Successfully downloaded file %s to %s directory.", treceive_filename, CLIENT_FILE_DIR);
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
		// UnlockClient();
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

	// Send Buffer -> data in bite-size packages
	char send_buffer[BUFFER_SIZE];

	// Store the file in a buffer
	FILE* file = fopen(filepath, "rb");
	char* file_buffer = malloc(filesize + 1);
	memset(file_buffer, 0, filesize + 1);
	
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
		for (int i = 0; i < (filesize / (int) BUFFER_SIZE) + 1; i++)
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
			DELAY;
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
#ifdef RESEND_ON_FALURE
			else if (strstr(read_buffer, RES_PLEASE_RESEND))
			{
				retVal = SendFile(server_file_descriptor, filepath, filesize);
			}
#endif
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

	// Receive buffer -> receive the data in bite-size packages because large files love to dunk on me.
	char rec_buffer[BUFFER_SIZE];

	// Determine the filepath (make it unique)
	char filepath[BUFFER_SIZE];
	sprintf(filepath, "%s%s", CLIENT_FILE_DIR, filename);
	int file_duplicate_check = 0;
	while (CheckIfFileExistsWithContent(filepath))
	{
		sprintf(filepath, "%s%s_%03d", CLIENT_FILE_DIR, filename, file_duplicate_check);
		file_duplicate_check++;
	}
	FILE* file = fopen(filepath, "wb");

	if (verbose)
		printf("Receiving file of size %d and saving as: %s\n", filesize, filepath);

	// Store the incoming contents in a buffer
	char* file_buffer = malloc(filesize + 1);
	memset(file_buffer, 0, filesize + 1);

	// Response (success or fail)
	char send_buffer[BUFFER_SIZE];

	if (file_buffer == NULL)
	{
		fprintf(stderr, "Error at line %d: failed to malloc.\n", __LINE__);

		// Tell the server to abandon shop
		sprintf(send_buffer, "%s", REQ_ABORT_RECEIVE);
		send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);
	}
	else
	{
		// Tell the server to start
		sprintf(send_buffer, "%s", REQ_READY_TO_RECEIVE);
		send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);

		// Read the incoming data in groups of BUFFER_SIZE
		int read_success = 1;
		for (int i = 0; i < (filesize / (int) BUFFER_SIZE) + 1; i++)
		{
			memset(rec_buffer, 0, BUFFER_SIZE);

			// Read the next set of BUFFER_SIZE
			int bSuccess = read(server_file_descriptor, rec_buffer, BUFFER_SIZE);
			if (bSuccess)
			{
				// Copy the data from the receive buffer to the file_buffer
				for (int j = 0; j < BUFFER_SIZE; j++)
				{
					if (i*BUFFER_SIZE + j < filesize)
					{
						file_buffer[i*BUFFER_SIZE + j] = rec_buffer[j];
					}
				}
			}
			else
			{
				read_success = 0;
			}
			DELAY;
		}

		if (read_success)
		{

#ifdef TEST_BINARY_READ
			// Test the binary read
			fprintf(stdout, "Reading binary: \n");
			for (int i = 0; i < filesize; i++)
			{
				fprintf(stdout, "%u", file_buffer[i]);
			}
#endif

			// Write the buffer to a file
			fwrite(file_buffer, sizeof(char), filesize, file);
			fflush(file);
			fclose(file);

			if (verbose)
				printf("Success! File %s saved to %s!\n", filename, filepath);

#ifdef RESEND_ON_FALURE
			char read_buffer[BUFFER_SIZE];
			if (GetFilesize(filepath) == 0)
			{
				// Something went wrong
				fprintf(stderr, "Error: filesize less than expected for %s.\n", filepath);

				// Reply the data is bbaaaad.
				memset(send_buffer, 0, BUFFER_SIZE);
				sprintf(send_buffer, "%s", REQ_BADDATA_RESEND);
				send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);

				// I expect a goodbye
				memset(read_buffer, 0, BUFFER_SIZE);
				read(server_file_descriptor, read_buffer, BUFFER_SIZE);

				// And here we go again
				retVal = ReceiveFile(server_file_descriptor, filename, filesize);
			}
			else
			{
				// Reply the data is good
				memset(send_buffer, 0, BUFFER_SIZE);
				sprintf(send_buffer, "%s", REQ_GOODDATA_END);
				send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);

				// I expect a goodbye
				memset(read_buffer, 0, BUFFER_SIZE);
				read(server_file_descriptor, read_buffer, BUFFER_SIZE);

				retVal = 1;
			}
#else
			retVal = 1;
#endif
		}
		else
		{
			// Something went wrong
			fprintf(stderr, "Error at line %d: failed to receice data.\n", __LINE__);
			retVal = -1;
		}
	}

	// Cleanup
	if (file_buffer)
	{
		free(file_buffer);
		file_buffer = NULL;
	}

	return retVal;
}


int RequestPort(int server_file_descriptor)
{
	// Client sends first, then reads

	char read_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];
	memset(read_buffer, 0, BUFFER_SIZE);
	memset(send_buffer, 0, BUFFER_SIZE);

	// Request a port
	sprintf(send_buffer, REQ_AVAILABLE_PORT);

	// Send request
	send(server_file_descriptor, send_buffer, BUFFER_SIZE, 0);

	// Read response
	if (read(server_file_descriptor, read_buffer, BUFFER_SIZE) < 0)
	{
		fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
	}
	else
	{
		if (verbose)
			printf("Got response: %s\n", read_buffer);

		if (strstr(read_buffer, RES_AVAILABLE_PORT))
		{
			char* port_token = GetParam(read_buffer, 1, " \n");

			int port = 0;
			int i = 0;
			while (port_token[i])
			{
				port = port * 10 + port_token[i] - '0';
				i++;
			}

			fprintf(stderr, "Received port: %d.\n", port);
			if (verbose)
				printf("Received port: %d.\n", port);

			return port;
		}
	}
	
	return 0;
}


void LockClient()
{
	while (CheckIfFileExists(LOCK_FILE))
	{
		//wait
	}

	FILE* LockFile = fopen(LOCK_FILE, "w");
	fprintf(LockFile, "Mine");
	fclose(LockFile);
}

void UnlockClient()
{
	while (CheckIfFileExists(LOCK_FILE))
	{
		remove(LOCK_FILE);
	}
}