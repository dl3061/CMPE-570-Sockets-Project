/*
	THelp.h
	Helper functions

	Author:
	David Lin (dl3061@rit.edu)
*/

#ifndef THELP_H
#define THELP_H

#include "TParam.h"

/*
	Verfies a given username and password
*/
int VerifyUser(char* username, char* password);

/*
	Checks if a file exists (and if it has read permission)
*/
int CheckIfFileExists(char* filename);

/*
	Checks if a file exists and has more than 0 bytes
*/
int CheckIfFileExistsWithContent(char* filename);

/*
	Gets the file size of a file
*/
int GetFilesize(char* filename);

#endif