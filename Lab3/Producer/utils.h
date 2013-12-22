// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// ErrorCodes enum and tokens used for director-producer communication.
//

#pragma once

#include <string>

using namespace std;

enum ErrorCodes
{
	Success = 0,
	ArgumentError,
	ListenError,
	CommunicationError,
	RegisterHandlerError,
	UnknownExceptionError
};

const unsigned short DEFAULT_PORT = 1025;

// Tokens to received from director
const char END_OF_PLAYS_TOKEN = 1;
const char PLAY_ENDED_TOKEN = 2;
const char PLAY_STARTED_TOKEN = 3;
const char DIRECTOR_TERMINATED_TOKEN = 4;

// Tokens sent to director
const char START_PLAY_TOKEN = 5;
const char STOP_PLAY_TOKEN = 6;
const char QUIT_TOKEN = 7;