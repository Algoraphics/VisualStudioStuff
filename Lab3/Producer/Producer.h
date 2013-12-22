// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Producer class.
//
#pragma once

#include "Acceptor.h"
#include "Console.h"
#include <map>
#include <memory>
#include <mutex>

using namespace std;

/*
 * A Producer is responsible for setting up connections between any number of Directors running on other machines.
 * It receives console input from a user via an input thread within the Console class, and from this input it can
 * reactively dispatch commands to its Directors to start or stop reciting a play, or to remove one or all 
 * connections it has to its Directors.
 */

// Used to handle SIGINT signals
void handleSignal(int signum);

class Producer : public ACE_Event_Handler
{
private:
	static shared_ptr<Producer> instance;

	Acceptor acceptor;
	Console console;
	map<int, shared_ptr<Connection>> directorConnections;
	map<int, vector<string>> directorRepertoires;
	map<int, pair<int, bool>> runningPlays;
	int nextDirectorId;
	bool terminate;
	mutex mut;

	Producer(unsigned short port, const char* ipAddr);
	void updateUI();

public:
	static void createInstance(unsigned short port, const char* ipAddr);
	static Producer *getInstance();

	void produce();
	void addDirector(shared_ptr<Connection> connection);
	void setRepertoire(int directorId, vector<string> plays);
	void directorStarted(int directorId);
	void directorReady(int directorId);
	void removeDirector(int directorId);
	void startPlay(int directorId, int playId);
	void stopPlay(int directorId);
	void quit();
	void notifyQuit();
	bool shouldQuit();

	virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;
};