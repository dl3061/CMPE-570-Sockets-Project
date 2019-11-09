/*
	Tiger.h
	David Lin (dl3061@rit.edu)
*/

#ifndef TIGER_H
#define TIGER_H

#define PORT	(8080)


#define BUFFER_SIZE	(1024)

#define CMD_TCONNECT	"tconnect"
#define CMD_TGET		"tget"
#define	CMD_TPUT		"tput"
//#define	CMD_END			"tend"
#define CMD_END			"exit"		// From bash script

#define BASE_SOCKET		(0)

// Unknown
#define	RES_UNKNOWN		"unknown"

// Okay
#define RES_OKAY		"okay"

// Authorization Success
#define RES_AUTH		"success_authorized"

// Authorization Failed
#define RES_AUTHFAILED	"failed_authorization"

// Not yet authorized - Must sign in first.
#define RES_UNAUTH		"unauthorized"

// Kill/end client
#define RES_ENDCLIENT	"endclient"
#define RES_KILLCIENT	RES_ENDCLIENT

// Ready to receive
#define RES_READY_TO_RECEIVE	"ready_receiving_next"
#define RES_RECEIVE_SUCCESS		"receiving_success"
#define RES_RECEIVE_FAILURE		"receiving_failed"

#define RES_READY_TO_SEND		"ready_sending_next"

#define T_CLEARSCREEN	"\033[2J\033[H"
#define T_SCROLLALOT	"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"


// Server file directory
#define SERVER_FILE_DIR		"files/"
#endif