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
#define	CMD_END			"tend"

#define	RES_UNKNOWN		"unknown"
#define RES_OKAY		"okay"
#define RES_AUTH		"authorized"
#define RES_UNAUTH		"unauthorized"
#define RES_ENDCLIENT	"endclient"
#define RES_KILLCIENT	RES_ENDCLIENT

#define T_CLEARSCREEN	"\033[2J\033[H"

#endif