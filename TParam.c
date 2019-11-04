/*
	TParams.c
	David Lin (dl3061@rit.edu)

	Helper function to parse param
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Tiger.h"
#include "TParam.h"

char* GetParam(char input[], int index, const char *delims)
{
	// Make a copy
	char input_buffer[BUFFER_SIZE];
	sprintf(input_buffer, "%s", input);

	char* token = strtok(input_buffer, delims);
	for (int i = 0; i < index; i++)
	{
		token = strtok(NULL, delims);
	}

	return token;
}
