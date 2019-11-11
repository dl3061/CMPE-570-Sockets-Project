/*
	TigerS.c
	Main program for the Tiger server.

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
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h> 
#include <pthread.h>

#include "Tiger.h"
#include "TParam.h"
#include "THelp.h"

// Prototypes for private functions
int MainProgramLoop(int client_file_descriptor);
void* MainProgramLoopThread(void* arg);
int ReceiveFile(int client_file_descriptor, char* filename, int filesize);
int SendFile(int client_file_descripttor, char* filepath, int filesize);

int verbose = 0;

// For threads
struct argMainProgramLoopThread {
	int thread_id;
	int client_file_descriptor;
};

pthread_mutex_t file_io_mutex;

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

	// Create the mutex for file IO operations
	if (pthread_mutex_init(&file_io_mutex, NULL) != 0)
	{
		fprintf(stderr, "Error at line %d: Mutex init has failed.\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	// Set verbose to true for debugging
	verbose = 1;

	// Apply highly advanced scientific technology 
	//	to accept incoming sockets and open a new thread for each
	int keep_server_alive = 1;
	pthread_t tid[MAX_THREADS];
	int session_cnt = 0;
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
			// Pass the arguments through an args struct
			//		Malloc it here so we're pointing at new memory each thread (no collision), 
			//		but each thread retains pointer to its respective malloced block.
			//		Free it at the end of each thread when it exists. 
			struct argMainProgramLoopThread* args = (struct argMainProgramLoopThread *) malloc(sizeof(struct argMainProgramLoopThread));
			args->thread_id = session_cnt + 1; // make it 1-index
			args->client_file_descriptor = new_socket_fd;

			pthread_create(&(tid[session_cnt]), NULL, &MainProgramLoopThread, (void *) args);

			session_cnt += 1;
		}

		if (session_cnt > MAX_THREADS)
			keep_server_alive = 0;
	}

	pthread_mutex_destroy(&file_io_mutex);

	return 0;
}


/*
	Wrapper for main program loop
*/
void* MainProgramLoopThread(void* arg)
{
	// Get args
	struct argMainProgramLoopThread* args = (struct argMainProgramLoopThread *) arg;
	int thread_id = args->thread_id;
	int client_file_descriptor = args->client_file_descriptor;
	
	// Print start
	printf("Connected: starting session #%d.\n", thread_id);

	int success = MainProgramLoop(client_file_descriptor);
	if (success)
	{
		if (verbose)
			printf("Exiting thread peacefully.\n");
	}

	// Print finish
	printf("Disconnected: ending session #%d.\n", thread_id);

	// Cleanup
	free(args);

	pthread_exit(NULL);
}

/*
	Main Program Loop
*/
int MainProgramLoop(int client_file_descriptor)
{
	int retVal = 0;

	// Server reads first, then sends
	int keep_program_alive = 1;
	char read_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];

	// IO
	char receive_filename[BUFFER_SIZE];
	int receive_filesize = 0;
	char send_filename[BUFFER_SIZE];
	int send_filesize = 0;

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
			if (verbose)
				printf("Recieved from client: %s \n", read_buffer);

			// TCONNECT - can run if user is unauthorized
			if (strstr(read_buffer, CMD_TCONNECT))
			{
				if (verbose)
					printf("Got a tconnect command!\n");

				// Check username and password
				char* username_token = GetParam(read_buffer, 1, " \n");
				char* password_token = GetParam(read_buffer, 2, " \n");

				if (verbose)
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
						if (verbose)
							printf("Got a tget command!\n");

						// Convert the filename to use server directory
						char* file_token = GetParam(read_buffer, 1, " \n");
						sprintf(send_filename, "%s%s", SERVER_FILE_DIR, file_token);
						
						// Check if it exists
						if (CheckIfFileExists(send_filename))
						{
							if (verbose)
								printf("Ready to send file: %s\n", send_filename);

							sending_file = 1;
							send_filesize = GetFilesize(send_filename);

							// Send filesize
							sprintf(send_buffer, "%s %d", RES_READY_TO_SEND, send_filesize);
						}
						else
						{
							sprintf(send_buffer, RES_CANNOTFINDFILE);
						}
					}

					// TPUT
					else if (strstr(read_buffer, CMD_TPUT))
					{
						if (verbose)
							printf("Got a tput command!\n");

						// Get the filename and filesize out of the command
						char* file_token = GetParam(read_buffer, 1, " \n");
						sprintf(receive_filename, "%s", file_token);

						// Highly advanced scientific technology to math the filesize
						char* size_token = GetParam(read_buffer, 2, " \n");
						int i = 0;
						receive_filesize = 0;
						while (size_token[i])
						{
							receive_filesize = receive_filesize * 10 + size_token[i] - '0';
							i++;
						}

						
						receive_file = 1;
						sprintf(send_buffer, RES_READY_TO_RECEIVE);
					}

					// TEND
					else if (strstr(read_buffer, CMD_END))
					{
						if (verbose)
							printf("Got an end command!\n");

						keep_program_alive = 0;
						retVal = 1;
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
		send(client_file_descriptor, send_buffer, BUFFER_SIZE, 0);

		// Temporarily interrupt loop to send/receive files
		if (receive_file)
			ReceiveFile(client_file_descriptor, receive_filename, receive_filesize);

		if (sending_file)
			SendFile(client_file_descriptor, send_filename, send_filesize);
	}

	return retVal;
}


/*
	Client used tput
	Recieve a file from the client and save to file/filename.
	Put will overwrite any existing file, like HTTP Put. 
*/
int ReceiveFile(int client_file_descriptor, char* filename, int filesize)
{
	int retVal = 0;

	// Receive buffer -> receive the data in bite-size packages because large files love to dunk on me.
	char rec_buffer[BUFFER_SIZE];

	// Determine the filepath 
	char filepath[BUFFER_SIZE];
	sprintf(filepath, "%s%s", SERVER_FILE_DIR, filename);
	
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
	}
	else
	{
		// Read the incoming data in groups of BUFFER_SIZE
		int read_success = 1;
		for (int i = 0; i < (filesize / BUFFER_SIZE) + 1; i++)
		{
			memset(rec_buffer, 0, BUFFER_SIZE);

			// Read the next set of BUFFER_SIZE
			int bSuccess = read(client_file_descriptor, rec_buffer, BUFFER_SIZE);
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
		}

		//if (read(client_file_descriptor, (void *)rec_buffer, (filesize + 1)))
		if (read_success)
		{
			/*
			// Unused because I though the error in my binary file was because it had nulls.
			// Turned out it was just because the file was too large to send at once.
			// Hacky thing to do -> "decrypt" so there are no zeroes
			for (int i = 0; i < filesize; i++)
			{
				uint8_t val1 = rec_buffer[i];
				uint8_t val2 = rec_buffer[i+1];

				char val = (val1 & 0xF0) | (val2 & 0x0F);


				file_buffer[i] = val;
			}
			file_buffer[filesize] = 0;

			*/

#ifdef TEST_BINARY_READ
			// Test the binary read
			fprintf(stdout, "Reading binary: \n");
			for (int i = 0; i < filesize; i++)
			{
				fprintf(stdout, "%u", file_buffer[i]);
			}
#endif
			
			// Write the buffer to a file (use mutex to prevent collision)
			pthread_mutex_lock(&file_io_mutex);
			FILE* file = fopen(filepath, "wb");
			fwrite(file_buffer, sizeof(char), filesize, file);
			fclose(file);
			pthread_mutex_unlock(&file_io_mutex);

			if (verbose)
				printf("Success! File %s saved to %s!\n", filename, filepath);

#ifdef RESEND_ON_FALURE

			if (GetFilesize(filepath) == 0)
			{
				// Something went wrong
				fprintf(stderr, "Error: filesize less than expected.");

				// Let the client know this was a triumph. I'm making a note here: huge success.
				sprintf(send_buffer, "%s", RES_PLEASE_RESEND);
				send(client_file_descriptor, send_buffer, BUFFER_SIZE, 0);

				// And here we go again
				retVal = ReceiveFile(client_file_descriptor, filename, filesize);
			}
			else
			{
				// Let the client know this was a triumph. I'm making a note here: huge success.
				sprintf(send_buffer, "%s", RES_RECEIVE_SUCCESS);
				send(client_file_descriptor, send_buffer, BUFFER_SIZE, 0);

				// It's hard to overstate my satisfaction
				retVal = 1;
			}
#else
			// Let the client know this was a triumph. I'm making a note here: huge success.
			sprintf(send_buffer, "%s", RES_RECEIVE_SUCCESS);
			send(client_file_descriptor, send_buffer, BUFFER_SIZE, 0);

			// It's hard to overstate my satisfaction
			retVal = 1;
#endif
		}
		else
		{
			fprintf(stderr, "Error at line %d: failed to receice data.\n", __LINE__);

			// Let the client know something went horribly worng.
			sprintf(send_buffer, "%s", RES_RECEIVE_FAILURE);
			send(client_file_descriptor, send_buffer, BUFFER_SIZE, 0);

			// 
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


/*
	Client used tget
	Send a file specified by the user.
*/
int SendFile(int client_file_descripttor, char* filepath, int filesize)
{
	int retVal = 0;
	
	// Buffer to send incoming data
	char read_buffer[BUFFER_SIZE];

	// Send Buffer -> data in bite-size packages
	char send_buffer[BUFFER_SIZE];

	// Store the file in a buffer
	char* file_buffer = malloc(filesize + 1);
	memset(file_buffer, 0, filesize + 1);

	if (file_buffer == NULL || send_buffer == NULL)
	{
		fprintf(stderr, "Error at line %d: failed to malloc.\n", __LINE__);
	}
	else
	{
		// Read - wait for start or abort
		memset(read_buffer, 0, BUFFER_SIZE);
		if (read(client_file_descripttor, read_buffer, BUFFER_SIZE) < 0)
		{
			fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
		}
		else
		{
			if (strstr(read_buffer, REQ_ABORT_RECEIVE))
			{
				// Abort
				retVal = -1;
			}
			else if (strstr(read_buffer, REQ_READY_TO_RECEIVE))
			{
				// Copy the contents into a buffer (use mutex to prevent collision)
				pthread_mutex_lock(&file_io_mutex);
				FILE* file = fopen(filepath, "rb");
				fread(file_buffer, sizeof(char), filesize, file);
				fclose(file);
				pthread_mutex_unlock(&file_io_mutex);

#ifdef TEST_BINARY_READ
				// Test the binary read
				fprintf(stderr, "Reading binary: \n");
				for (int i = 0; i < filesize; i++)
				{
					fprintf(stderr, "%u", file_buffer[i]);
				}
				fprintf(stderr, "\n\n");
#endif

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
					send(client_file_descripttor, send_buffer, BUFFER_SIZE, 0);
				}

#ifdef RESEND_ON_FALURE
				// Read client response
				memset(read_buffer, 0, BUFFER_SIZE);
				if (read(client_file_descripttor, read_buffer, BUFFER_SIZE) < 0)
				{
					fprintf(stderr, "Error at line %d: file/stream read failure.\n", __LINE__);
				}
				else
				{
					if (strstr(read_buffer, REQ_BADDATA_RESEND))
					{
						// Reply okay
						memset(send_buffer, 0, BUFFER_SIZE);
						sprintf(send_buffer, "%s", RES_OKAY);
						send(client_file_descripttor, send_buffer, BUFFER_SIZE, 0);

						// And here we go again;
						retVal = SendFile(client_file_descripttor, filepath, filesize);
					}
					else if(strstr(read_buffer, REQ_GOODDATA_END))
					{
						// Reply goodbye
						memset(send_buffer, 0, BUFFER_SIZE);
						sprintf(send_buffer, "%s", RES_OKAY);
						send(client_file_descripttor, send_buffer, BUFFER_SIZE, 0);

						retVal = 1;
					}
				}
#else
				retVal = 1;
#endif
				
				// Done
				if (verbose)
					printf("Success! File %s sent to the client!\n", filepath);
			}
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