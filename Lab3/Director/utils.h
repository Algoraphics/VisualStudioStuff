// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// ErrorCodes enum, string functions and tokens to be sent between the director and the producer.
//

#pragma once

#include <string>

using namespace std;

enum ErrorCodes
{
	Success = 0,
	ArgumentError,
	FileOpenError,
	NoCharactersError,
	PlayerCurrentlyPerformingPlayError,
	ConnectionError,
	CommunicationError,
	RegistrationError,
	UnknownExceptionError
};

const unsigned short DEFAULT_PORT = 1025;

// Tokens to send to producer
const char END_OF_PLAYS_TOKEN = 1;
const char PLAY_ENDED_TOKEN = 2;
const char PLAY_STARTED_TOKEN = 3;
const char DIRECTOR_TERMINATED_TOKEN = 4;

// Tokens received from producer
const char START_PLAY_TOKEN = 5;
const char STOP_PLAY_TOKEN = 6;
const char QUIT_TOKEN = 7;

/*
 * Removes whitespace from the front of a string.
 * Modifies the string argument by reference and 
 * returns the result.
 */
string& trim(string& s);


/*
 * Removes a file extension from the end of a string.
 * Modifies the string argument by reference and 
 * returns the result.
 */
string& stripExt(string& s);