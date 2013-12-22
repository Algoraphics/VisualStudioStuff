// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Director class.
//

#pragma once

#include "Play.h"
#include "Player.h"
#include "CharacterPartQueue.h"
#include <string>
#include <vector>
#include <memory>
#include <ace/Event_Handler.h>
#include <ace/INET_Addr.h>
#include <ace/SOCK_Connector.h>
#include <signal.h>
#include <atomic>

using namespace std;

class Play;

// Used to handle SIGINT signals
void handleSignal(int signum);

/*
 * A Director is responsible for parsing a set of script files and populating
 * a queue of CharacterParts that Player threads can grab from in a
 * greedy manner. The data for each play is wrapped in a Play object and stored
 * in a map. The Director also creates the Players that can perform the plays.
 * Once a Director is set up, it communicates with a producer process reactively,
 * cueing plays, stopping plays or terminating the process when requested.
 *
 * The director class is used through a singleton instance in each director process.
 */
class Director : ACE_Event_Handler
{
private:
	static shared_ptr<Director> instance;

	vector<shared_ptr<Player>> players;
	map<string, shared_ptr<Play>> plays;
	atomic<bool> performingPlay, stop, terminate;
	string currentPlay;
	bool waitingForPlayName, reactorStopped;
	string playNameRead;
	ACE_SOCK_Stream stream;

	Director(unsigned short port, string ipAddress, vector<string> & scriptFileNames, unsigned minRequestedPlayers = 0);
	void cue(string playName);
	void stopPlay();

	// Prevent compiler implementations (do not use)
	Director(const Director & other);
	void operator=(const Director & other);

public:
	static void createInstance(unsigned short port, string ipAddress, vector<string> & scriptFileNames, unsigned minRequestedPlayers = 0);
	static Director *getInstance();

	~Director();
	void notifyPlayStarted();
	void notifyPlayFinished();
	bool shouldStop();
	bool shouldTerminate();
	void direct();
	void terminateTroupe();

	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;
	virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;
};