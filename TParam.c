/*
	TParams.c
	David Lin (dl3061@rit.edu)

	Helper function to parse param
*/

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Tiger.h"
#include "TParam.h"

char* GetParam(char input[], int index, const char *delims)
{
	char *saveptr;

	// Make a copy
	char input_buffer[BUFFER_SIZE];
	sprintf(input_buffer, "%s", input);

	char* token = strtok_r(input_buffer, delims, &saveptr);
	for (int i = 0; i < index; i++)
	{
		token = strtok_r(NULL, delims, &saveptr);
	}

	return token;
}
