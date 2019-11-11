/*
	THelp.c
	Helper functions

	Author:
	David Lin (dl3061@rit.edu) 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "TParam.h"


/*
	Verfies a given username and password
*/
int VerifyUser(char* username, char* password)
{
	int user_verified = 0;
	printf("Checking credentials for %s and %s.\n", username, password);

	// Hardcode
	if (strstr(username, "user") && strstr(password, "pass"))
	{
		user_verified = 1;
	}

	if (user_verified)
	{
		printf("User %s verified! Signing in.\n", username);
	}
	return user_verified;
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
	Gets the file size of a file
*/
int GetFilesize(char* filename)
{
	int filesize = 0;

	if (CheckIfFileExists(filename))
	{
		// Open the file
		FILE* file = fopen(filename, "r");

		if (file != NULL)
		{
			// Seek the filesize
			fseek(file, 0L, SEEK_END);
			filesize = ftell(file);

			// Close the file
			fclose(file);
		}
	}

	return filesize;
}