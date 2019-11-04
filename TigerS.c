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

// Prototypes for private functions
int MainProgramLoop(int client_file_descriptor);
int HandleSocketConnection(int file_descriptor);

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

	while (keep_program_alive)
	{
		// Read incoming command and save it to the buffer
		memset(read_buffer, 0, BUFFER_SIZE);
		if (read(client_file_descriptor, read_buffer, BUFFER_SIZE) < 0)
		{
			fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
		}
		else
		{
			// Find corresponding command
			if (strstr(read_buffer, CMD_TCONNECT))
			{
				printf("Got a tconnect command!\n");

				sprintf(send_buffer, RES_AUTH);
			}
			else if (strstr(read_buffer, CMD_TGET))
			{
				printf("Got a tget command!\n");
			}
			else if (strstr(read_buffer, CMD_TPUT))
			{
				printf("Got a tput command!\n");
			}
			else if (strstr(read_buffer, CMD_END))
			{
				printf("Got an end command!\n");

				keep_program_alive = 0;
				sprintf(send_buffer, RES_ENDCLIENT);
			}
			else
			{
				sprintf(send_buffer, "Hello from server!");
			}
		}

		// Send response status 
		send(client_file_descriptor, send_buffer, strlen(send_buffer), 0);
	}
	return 0;
}

int HandleSocketConnection(int client_file_descriptor)
{
	char buffer[BUFFER_SIZE] = { 0 };

	// Try to read the stream from the client
	if (read(client_file_descriptor, buffer, BUFFER_SIZE) < 0)
	{
		fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
		return (-1);
	}

	// Print the data in
	printf("%s\n", buffer);
	
	// Send response
	char *hello = "Hello from server!\n";
	send(client_file_descriptor, hello, strlen(hello), 0);

	// Try to read the stream from the client
	if (read(client_file_descriptor, buffer, BUFFER_SIZE) < 0)
	{
		fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
		return (-1);
	}
	printf("%s\n", buffer);

	// Send response
	send(client_file_descriptor, hello, strlen(hello), 0);

	return (1);
}