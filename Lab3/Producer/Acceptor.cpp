// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Acceptor class definition.
//

#include "stdafx.h"
#include "Acceptor.h"
#include "utils.h"
#include "Producer.h"
#include <iostream>

using namespace std;

/*
 *	Opens the Acceptor and registers a handler for the Acceptor with the singleton reactor instance.
 */
Acceptor::Acceptor(unsigned short port, const char *ipAddr, Producer & producer_)
	: producer(producer_)
{
	ACE_INET_Addr address(port, ipAddr);
	if (this->acceptor.open(address, 1) >= 0)
	{
		if (ACE_Reactor::instance()->register_handler(this->acceptor.get_handle(), this, ACCEPT_MASK) < 0)
		{
			cerr << "Register Handler for Acceptor Constructor failed." << endl;
			throw RegisterHandlerError;
		}
	}
	else
	{
		cerr << "Acceptor failed to open" << endl;
		throw ListenError;
	}
}

/*
 *	Explicity remove the handler on destruction and close the acceptor.
 */
Acceptor::~Acceptor()
{
	if (ACE_Reactor::instance()->remove_handler(this->acceptor.get_handle(), ACCEPT_MASK) < 0)
	{
		cerr << "Removing acceptor handler failed" << endl;
	}

	this->acceptor.close();
}

/*
 *	Upon receiving acceptable input from a director, create a Connection object and
 *	call addDirector to add it to the producer
 */
int Acceptor::handle_input(ACE_HANDLE h)
{
	shared_ptr<Connection> connectionPtr(new Connection(this->producer));
	if (this->acceptor.accept(connectionPtr->getStream()) >= 0)
	{
		this->producer.addDirector(connectionPtr);
		if (ACE_Reactor::instance()->register_handler(connectionPtr->getStream().get_handle(), connectionPtr.get(), READ_MASK) < 0)
		{
			cerr << "Register Handler for connection stream failed." << endl;
		}
	}
	else
	{
		cerr << "Acceptor failed to accept director connection attempt" << endl;
	}

	return 0;
}