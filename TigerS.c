/*
	TigerS.c
	David Lin (dl3061@rit.edu)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 

#include "Tiger.h"
#include "TParam.h"

// Prototypes for private functions
int MainProgramLoop(int client_file_descriptor);
int VerifyUser(char* username, char* password);

int ReceiveFile(int client_file_descriptor, char* filename, int filesize);

int main()
{
	// Create the socket file descriptor
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		// AF_INET -> IPV4 protocol
		// SOCK_STREAM -> TCP
		// Protocol -> 0 (Internet Protocol)
	if (socket_fd == 0)
	{
		fprintf(stderr, "Error at line %d: Creating socket failed.\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	// Set socket options
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		fprintf(stderr, "Error at line %d: setsockopt failed.\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	// Address(?)
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Bind the socket
	if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		fprintf(stderr, "Error at line %d: bind failure.\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	// Begin listening.
	if (listen(socket_fd, 3) < 0)
	{
		fprintf(stderr, "Error at line %d: listen failure.\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	// Apply highly advanced scientific technology to accept incoming sockets and open a new thread for each
	int keep_server_alive = 1;
	while (keep_server_alive)
	{
		// Keep on trying to accept socket connections

		// The accept() call creates a new socket descriptor with the same properties as socket and returns it to the caller. 
		int new_socket_fd = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		if (new_socket_fd < 0)
		{
			fprintf(stderr, "Error at line %d: accept failure.\n", __LINE__);
		}
		else
		{
			// @todo Start a new thread

			printf("Connected: starting session.\n");
			MainProgramLoop(new_socket_fd);
			printf("Disconnected: ending session.\n");
		}
	}

	return 0;
}

int MainProgramLoop(int client_file_descriptor)
{
	// Server reads first, then sends
	int keep_program_alive = 1;
	char read_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];

	char receive_filename[BUFFER_SIZE];
	int receive_filesize;
	char send_filename[BUFFER_SIZE];

	// Variables
	int user_is_authorized = 0;

	while (keep_program_alive)
	{
		int receive_file = 0;
		int sending_file = 0;

		// Read incoming command and save it to the buffer
		memset(read_buffer, 0, BUFFER_SIZE);
		if (read(client_file_descriptor, read_buffer, BUFFER_SIZE) < 0)
		{
			fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
		}
		else
		{
			// Find corresponding command
			printf("Recieved from client: %s \n", read_buffer);

			// TCONNECT - can run if user is unauthorized
			if (strstr(read_buffer, CMD_TCONNECT))
			{
				printf("Got a tconnect command!\n");

				// Check username and password
				char* username_token = GetParam(read_buffer, 1, " \n");
				char* password_token = GetParam(read_buffer, 2, " \n");

				printf("Username: %s\tPassword: %s\n", username_token, password_token);

				if (VerifyUser(username_token, password_token))
				{
					// Success -> return auth okay
					user_is_authorized = 1;
					sprintf(send_buffer, RES_AUTH);
				}
				else
				{
					// Failed -> return cannot auth
					sprintf(send_buffer, RES_AUTHFAILED);
				}
			}
			else 
			{
				// Following cannot run if user is unauthorized
				if (user_is_authorized)
				{
					// TGET
					if (strstr(read_buffer, CMD_TGET))
					{
						printf("Got a tget command!\n");

						char* file_token = GetParam(read_buffer, 1, " \n");
						printf("Ready to send file: %s\n", file_token);
						sprintf(send_filename, "%s", file_token);

						sending_file = 1;
						sprintf(send_buffer, RES_READY_TO_SEND);
					}
					// TPUT
					else if (strstr(read_buffer, CMD_TPUT))
					{
						printf("Got a tput command!\n");

						char* file_token = GetParam(read_buffer, 1, " \n");
						sprintf(receive_filename, "%s", file_token);

						char* size_token = GetParam(read_buffer, 2, " \n");
						int i = 0;
						receive_filesize = 0;
						while (size_token[i])
						{
							receive_filesize = receive_filesize * 10 + size_token[i] - '0';
							i++;
						}

						// printf("Ready to download file: %s (size: %d)\n", receive_filename);

						receive_file = 1;
						sprintf(send_buffer, RES_READY_TO_RECEIVE);
					}
					// TEND
					else if (strstr(read_buffer, CMD_END))
					{
						printf("Got an end command!\n");

						keep_program_alive = 0;
						sprintf(send_buffer, RES_ENDCLIENT);
					}
					else
					{
						sprintf(send_buffer, RES_UNKNOWN);
					}
				}
				else
				{
					sprintf(send_buffer, RES_UNAUTH);
				}
			}
		}

		// Send response status 
		send(client_file_descriptor, send_buffer, strlen(send_buffer), 0);

		// Temporarily interrupt loop to send/receive files
		if (receive_file)
			ReceiveFile(client_file_descriptor, receive_filename, receive_filesize);
	}
	return 0;
}


int VerifyUser(char* username, char* password)
{
	int user_verified = 0;
	printf("Checking credentials for %s and %s.\n", username, password);

	// Hardcode
	user_verified = 1;

	if (user_verified)
	{
		printf("User %s verified! Signing in.\n", username);
	}
	return user_verified;
}

int ReceiveFile(int client_file_descriptor, char* filename, int filesize)
{
	int retVal = 0;

	// Response (success or fail)
	char send_buffer[BUFFER_SIZE];

	// File 
	char filepath[BUFFER_SIZE];
	sprintf(filepath, "%s%s", SERVER_FILE_DIR, filename);
	FILE* file = fopen(filepath, "w");
	
	printf("Receiving file of size %d and saving as: %s\n", filesize, filepath);
	
	// Store the incoming contents in a buffer
	char* file_buffer = malloc(filesize + 1);
	if (file_buffer)
	{
		if (read(client_file_descriptor, file_buffer, filesize + 1))
		{
			fprintf(file, "%s", file_buffer);
			fclose(file);

			sprintf(send_buffer, "%s", RES_RECEIVE_SUCCESS);
			retVal = 1;

			// Send the response
			send(client_file_descriptor, send_buffer, strlen(send_buffer), 0);
		}
		else
		{
			fprintf(stderr, "Error at line %d: failed to receice data.\n", __LINE__);

			sprintf(send_buffer, "%s", RES_RECEIVE_FAILURE);
			retVal = -1;

			// Send the response
			send(client_file_descriptor, send_buffer, strlen(send_buffer), 0);
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