#pragma once
// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Connection class.
//

#include <ace/Event_Handler.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Reactor.h>
#include <sstream>
#include <iostream>
#include <vector>
#include "utils.h"

class Producer;

/*
 * The Connection class wraps an ACE_SOCK_Stream and serves as a representation of a given
 * director connected to this producer. It contains a communication protocol for interaction
 * with said director and stores the director's ID and play names for display.
 */

class Connection : public ACE_Event_Handler
{
private:
	bool receivingPlays;
	ACE_SOCK_Stream stream;
	vector<string> playNames;
	string curPlayName;
	Producer & producer;
	int directorId;

public:
	Connection(Producer & producer_);
	virtual int handle_input(ACE_HANDLE h = ACE_INVALID_HANDLE) override;
	ACE_SOCK_Stream& getStream();
	void setDirectorId(int directorId);
};