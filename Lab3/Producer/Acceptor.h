// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Acceptor class.
//

#pragma once
#include <ace/Event_Handler.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Reactor.h>
#include <ace/INET_Addr.h>
#include "Connection.h"
#include <vector>
#include <memory>

class Producer;

/* 
 * The Acceptor class wraps the ACE_SOCK_Acceptor class, allowing us to accept input from
 * new directors and use that input to add the directors to the producer.
 */

class Acceptor : public ACE_Event_Handler
{
private:
	ACE_SOCK_Acceptor acceptor;
	Producer & producer;

public:
	Acceptor(unsigned short port, const char *ipAddr, Producer & producer_);
	virtual int handle_input(ACE_HANDLE h = ACE_INVALID_HANDLE) override;
	Acceptor::~Acceptor();
};