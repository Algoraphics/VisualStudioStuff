// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Console class definition.
//

#include "stdafx.h"
#include "Console.h"
#include "utils.h"
#include "Producer.h"

using namespace std;

Console::Console(Producer & producer_) : consoleState(state::noCommand), producer(producer_)
{
}

/*
 * Join the console's input thread on destruction
 */ 
Console::~Console()
{
	if (this->consoleInputThread.joinable())
	{
		this->consoleInputThread.join();
	}
}

/*
 * Main loop for the console, to be run within a thread when the console is started. Until the producer is shut down,
 * the console will repeatedly display the state of the producer and its connected directors, as well as a list of
 * possible user commands. It will then ask the user for input, and upon receiving it parse this input and call
 * the appropriate function on the producer. The console serves as a state machine, and the state is updated repeatedly
 * based on input in this function.
 */
void Console::runConsole()
{
	while (!this->producer.shouldQuit())
	{
		char buffer = cin.get();
		if (buffer == EOF) continue;

		//begin a command
		if (buffer == '<' && this->consoleState == state::noCommand)
		{
			this->consoleState = state::receivingCommand;
			this->currentCommand = "";
		}

		//finished receiving valid command
		else if (buffer == '>' && this->consoleState == state::receivingCommand)
		{
			if (currentCommand == "start")
			{
				this->consoleState = state::receivingValue;
				this->currentPlayNum = "";
			}
			else if (currentCommand == "stop")
			{
				this->consoleState = state::receivingValue;
				this->currentPlayNum = "";
			}
			else if (currentCommand == "quit")
			{
				this->consoleState = state::quit;
			}
			else
			{
				this->consoleState = state::invalidCommand;
			}
		}

		//end of console input
		else if (buffer == '\r' || buffer == '\n')
		{
			if (this->consoleState == state::quit)
			{
				this->producer.quit();
			}
			else if (this->consoleState == state::receivingCommand || this->consoleState == state::invalidCommand)
			{
				cerr << "Invalid Command." << endl;
			}
			else if (this->consoleState == state::receivingValue)
			{
				istringstream convertStream(this->currentPlayNum);
				int dirIndex;
				int playIndex;
				
				if (currentCommand == "start")
				{
					if (convertStream >> dirIndex && convertStream >> playIndex)
					{
						this->producer.startPlay(dirIndex, playIndex);
					}
					else
					{
						cerr << "Invalid director or play index given" << endl;
					}
				}
				else if (currentCommand == "stop")
				{
					if (convertStream >> dirIndex)
					{
						this->producer.stopPlay(dirIndex);
					}
					else
					{
						cerr << "Invalid director index given" << endl;
					}
				}
				else
				{
					cerr << "Invalid command." << endl;
				}
			}

			this->consoleState = state::noCommand;
			this_thread::sleep_for(chrono::milliseconds(250)); // small delay to avoid issues with rapid input
		}

		//these cases are if the input character is not a special token
		else if (this->consoleState == state::receivingCommand)
		{
			this->currentCommand += buffer;
		}
		else if (this->consoleState == state::receivingValue)
		{
			this->currentPlayNum += buffer;
		}
		else
		{
			this->consoleState = state::invalidCommand;
		}
	}
}

/*
 * Allow for the console to be explicitly started after creation
 */
void Console::startConsole()
{
	// Don't start the console more than once
	if (this->consoleInputThread.joinable())
	{
		return;
	}

	// Start the console's input thread
	this->consoleInputThread = thread([this]
	{ 
		this->runConsole();
	});
}