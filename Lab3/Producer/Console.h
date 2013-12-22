// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Console class.
//

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

class Producer;

using namespace std;

/*
 * The console class represents the actual input console with which the user will interact. It opens up
 * a thread on which the user is asked repeatedly for input, which is parsed by the main thread and used
 * to call the appropriate command, or respond with the appropriate error message.
 */

class Console
{
private:
	enum class state {receivingCommand, receivingValue, noCommand, quit, invalidCommand};
	state consoleState;
	string currentCommand;
	string currentPlayNum;
	Producer & producer;
	thread consoleInputThread;

	void runConsole();

public:
	Console(Producer & producer_);
	~Console();
	void startConsole();
};