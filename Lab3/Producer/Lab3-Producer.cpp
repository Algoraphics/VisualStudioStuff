// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu 
// 
// main function.
//

#include "stdafx.h"
#include "Producer.h"
#include "ace/INET_Addr.h"
#include <thread>

/*
 * Usage function to notify the user of incorrectly specified command line arguments.
 */
void usage(char* progName)
{
	cerr << "Usage: " << progName << " [port] [ip_address]" << endl;
}

int main(int argc, char* argv[])
{
	// Command line arguments
	enum {
		programNameIndex = 0,
		portIndex,
		ipIndex
	};

	const unsigned maxExpectedArgs = 3;

	try
	{
		if (argc > maxExpectedArgs)
		{
			usage(argv[programNameIndex]);
			cerr << "Too many arguments, last " << (argc - maxExpectedArgs) << " ignored" << endl;
		}

		// Use localhost as the default ip address for the producer
		string ipAddress(ACE_LOCALHOST);
		if (argc >= maxExpectedArgs)
		{
			ipAddress = (ipAddress == "0.0.0.0") ? ACE_LOCALHOST : ipAddress;
		}

		unsigned short port = DEFAULT_PORT;

		if (argc > portIndex)
		{
			// Parse the port argument
			istringstream portStream(argv[portIndex]);
			if (!(portStream >> port))
			{
				usage(argv[programNameIndex]);
				return ArgumentError;
			}
		}

		// Construct and start the producer
		Producer::createInstance(port, ipAddress.c_str());
		Producer::getInstance()->produce();
	}
	catch(ErrorCodes& ec)
	{
		return ec;
	}
	catch (...)
	{
		return UnknownExceptionError;
	}

	return Success;
}

