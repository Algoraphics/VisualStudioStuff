// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// main function.
//

#include "stdafx.h"
#include "utils.h"
#include "Director.h"
#include <sstream>
#include <vector>

/*
 * Usage function to notify the user of incorrectly specified command line arguments.
 */
void usage(char* progName)
{
	cerr << "Usage: " << progName << " <port> <ip_address> <min_threads> <script_file_name>+" << endl;
}

int main(int argc, char* argv[])
{
	enum {
		programNameIndex = 0,
		portIndex,
		ipIndex,
		minPlayersIndex,
		firstScriptFileIndex,
	};

	const unsigned minRequiredArgs = 5;

	try
	{
		// Verify the correct number of arguments were passed
		if (argc < minRequiredArgs)
		{
			usage(argv[programNameIndex]);
			return ArgumentError;
		}

		unsigned minPlayers = 0;
		unsigned short port = 0;
		string ipAddress(argv[ipIndex]);
		vector<string> scriptFiles;

		// Parse the min players and port arguments
		istringstream minPlayersStream(argv[minPlayersIndex]);
		istringstream portStream(argv[portIndex]);
		if (!(minPlayersStream >> minPlayers && portStream >> port))
		{
			usage(argv[programNameIndex]);
			return ArgumentError;
		}

		// Parse all of the script file names
		for (int i = firstScriptFileIndex; i < argc; ++i)
		{
			scriptFiles.push_back(argv[i]);
		}

		// Construct and start the director
		Director::createInstance(port, ipAddress, scriptFiles, minPlayers);
		Director::getInstance()->direct();
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
