/*
	Tiger.h
	David Lin (dl3061@rit.edu)
*/

#ifndef TIGER_H
#define TIGER_H

// #define TEST_BINARY_READ

#define PORT	(8080)
#define BASE_SOCKET		(0)

#define BUFFER_SIZE	(1024)
#define MAX_THREADS	(1000)

/* COMMANDS */

// User commands
#define CMD_TCONNECT	"tconnect"
#define CMD_TGET		"tget"
#define	CMD_TPUT		"tput"
//#define	CMD_END			"tend"
#define CMD_END			"exit"		// From bash script


/* CLIENT REQUESTS */

// Custom commands
#define REQ_TCONNECT	CMD_TCONNECT
#define REQ_TGET		CMD_TGET
#define REQ_TPUT		CMD_TPUT
#define REQ_END			CMD_END

#define REQ_READY_TO_RECEIVE	"ready_receiving_tget"
#define REQ_ABORT_RECEIVE		"not_ready_stop_that_plz"


/* SERVER RESPONSES */

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
#define RES_READY_TO_RECEIVE	"ready_receiving_tput"
#define RES_RECEIVE_SUCCESS		"receiving_success"
#define RES_RECEIVE_FAILURE		"receiving_failed"

#define RES_READY_TO_SEND		"ready_sending_next"
#define RES_SEND_SUCCESS		"sending_success"
#define RES_SEND_FAILURE		"sending_failed"
#define RES_CANNOTFINDFILE		"send_cannot_find_file"


/* TEXT */

#define T_CLEARSCREEN	"\033[2J\033[H"
#define T_SCROLLALOT	"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"


/* DIRECTORIES */
#define SERVER_FILE_DIR		"TigerS/"
#define CLIENT_FILE_DIR		"Downloads/"


#endif