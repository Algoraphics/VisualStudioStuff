// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu 
// 
// Director class definition.
//

#include "stdafx.h"
#include "Director.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ace/Reactor.h>

shared_ptr<Director> Director::instance;

/*
 *	Create the singleton instance.
 */
void Director::createInstance(unsigned short port, string ipAddress, vector<string> & scriptFileNames, unsigned minRequestedPlayers)
{
	Director::instance = shared_ptr<Director>(new Director(port, ipAddress, scriptFileNames, minRequestedPlayers));
}

/*
 *	Get the singleton instance.
 */
Director *Director::getInstance()
{
	return Director::instance.get();
}

/*
 * Constructs a Director by parsing the specified set of scripts and creating a queue
 * of parts for each that must be assigned to players in order. Constructs
 * Player objects that will take parts from a parts queue in a 
 * greedy manner once a play starts. Also constructs a Play object for each script
 * used to synchronize the Players.
 */
Director::Director(unsigned short port, string ipAddress, vector<string> & scriptFileNames, unsigned minRequestedPlayers)
	: performingPlay(false), stop(false), terminate(false), waitingForPlayName(false), reactorStopped(false)
{
	unsigned minRequiredPlayers = minRequestedPlayers;

	// Iterate through all of the requested scripts
	for (string& scriptFileName : scriptFileNames)
	{
		string playName(scriptFileName);
		stripExt(playName);

		this->plays[playName] = shared_ptr<Play>(new Play(*this));

		// Open the script file
		ifstream scriptFile(scriptFileName);
		if (!scriptFile.is_open())
		{
			cerr << "Could not open file " << scriptFileName << endl;
			throw FileOpenError;
		}

		bool isPrevLineScene = false;
		unsigned prevConfigNumPlayers = 0;
		unsigned sceneFragment = 1;

		// For each line of the script file
		string scriptFileLine;
		while (getline(scriptFile, scriptFileLine))
		{
			string firstToken;
			istringstream scriptFileLineStream(scriptFileLine);
			if (scriptFileLineStream >> firstToken)
			{
				if (firstToken == "[scene]")
				{
					// Add the scene title
					string sceneTitle;
					getline(scriptFileLineStream, sceneTitle);
					this->plays[playName]->getSceneTitles().push_back(trim(sceneTitle));
					isPrevLineScene = true;
				}
				else
				{
					// Open the configuration file
					ifstream configFile(firstToken);
					if (!configFile.is_open())
					{
						cerr << "Could not open file " << firstToken << endl;
						throw FileOpenError;
					}

					// Count players in the config file
					unsigned configNumPlayers = 0;
					unsigned lineCounter = 1;
					string configFileLine;
					while (getline(configFile, configFileLine))
					{
						if (trim(configFileLine).empty()) continue;

						string characterName, characterFile;
						istringstream configFileLineStream(configFileLine);
						if (configFileLineStream >> characterName && configFileLineStream >> characterFile)
						{
							CharacterPart part(sceneFragment, characterName, characterFile);
							this->plays[playName]->getCharacterParts().addPart(part);
							++configNumPlayers;
						} 
						else 
						{
							cerr << "Could not parse character definition at line: " << lineCounter << endl;
						}

						++lineCounter;
					}

					// Update minRequiredPlayers based upon the number of players in this config file and the preceeding config file.
					minRequiredPlayers = max(prevConfigNumPlayers + configNumPlayers, minRequiredPlayers);
					prevConfigNumPlayers = configNumPlayers;

					if (configNumPlayers == 0)
					{
						cerr << "No valid characters found for a scene." << endl;
						throw NoCharactersError;
					}

					// If the previous line was another config file, not a new scene, add an empty scene title.
					if (!isPrevLineScene)
					{
						this->plays[playName]->getSceneTitles().push_back("");
					}

					++sceneFragment;
					isPrevLineScene = false;
				}
			} 
		}
	}

	// Initiate connection to producer
	ACE_SOCK_Connector connector;
	port = (port == 0) ? DEFAULT_PORT : port; 
	const char* aceIpAddr = (ipAddress == "0.0.0.0") ? ACE_LOCALHOST : ipAddress.c_str();
	ACE_INET_Addr address(port, aceIpAddr);

	if (connector.connect(this->stream, address) < 0)
	{
		cerr << "Failed to connect to producer" << endl;
		throw ConnectionError;
	}

	// Register the socket with the singleton reactor instance to listen for read events
	if (ACE_Reactor::instance()->register_handler(this->stream.get_handle(), this, READ_MASK) < 0)
	{
		cerr << "Socket handler failed to register" << endl;
		throw RegistrationError;
	}

	// Register a signal handler with the singleton reactor instance
	signal(SIGINT, handleSignal);

	// Send play names to producer
	for (string & scriptFileName : scriptFileNames)
	{
		string playName(scriptFileName);
		stripExt(playName);
		playName += " ";

		if (stream.send_n(playName.c_str(), playName.size()) < 0)
		{
			cerr << "Failed to send play name to producer" << endl;
			throw CommunicationError;
		}
	}

	// Tell the producer that we are done sending plays
	if (stream.send_n(&END_OF_PLAYS_TOKEN, 1) < 0)
	{
		cerr << "Failed to send end of names token" << endl;
		throw CommunicationError;
	}

	// Start the players
	for (unsigned i = 0; i < minRequiredPlayers; ++i)
	{
		shared_ptr<Player> player(new Player(this->plays, *this));
		player->startPlayer();
		this->players.push_back(player);
	}
}

/*
 * Director destructor closes the connection to the producer and deregisters reactor handlers.
 */
Director::~Director()
{
	if (ACE_Reactor::instance()->remove_handler(this->stream.get_handle(), READ_MASK) < 0)
	{
		cerr << "Reactor failed to remove socket handler" << endl;
	}

	this->stream.close();

	// Wait for player threads to terminate before destroying member variables
	for (shared_ptr<Player> player : this->players)
	{
		player->waitForPlayerTerminate();
	}
}

/*
 * Updates the current play and notifies all of the player objects
 * that there is a new play to perform. Once all the players have been
 * notified, the Director notifies the producer that the play has started.
 * Does nothing if the Director is unable to perform the requested play.
 */
void Director::cue(string playName)
{
	//Check that we can actually start the requested play
	if (this->performingPlay)
	{
		cerr << "This director is already performing a play" << endl;
		return;
	}

	if (this->plays.count(playName) == 0)
	{
		cerr << "Play: " << playName << " does not exist" << endl;
		return;
	}

	this->stop = false;
	this->currentPlay = playName;
	this->plays[this->currentPlay]->reset();

	// Tell the players to begin the assigned play
	for (shared_ptr<Player> player : this->players)
	{
		player->beginPlay(playName);
	}

	// Notify the producer that we have started a play
	if (stream.send_n(&PLAY_STARTED_TOKEN, 1) < 0)
	{
		cerr << "Failed to send play started token" << endl;
	}
}

/*
 * Stops the current play by setting the stop flag and telling the play object
 * to notify all of the players.
 */
void Director::stopPlay()
{
	this->stop = true;
	ACE_Reactor::instance()->notify(this);
}

/*
 * Starts the termination protocol by setting the terminate flag and stopping 
 * the current play.
 */
void Director::terminateTroupe()
{
	this->terminate = true;
	this->stop = true;
	ACE_Reactor::instance()->notify(this);
}

/*
 * Sets the flag that signals that a play is being performed.
 */
void Director::notifyPlayStarted()
{
	this->performingPlay = true;
}

/*
 * Called asynchronously by a play on completion to wake
 * the director from its reactor event loop so it can
 * inform the producer of completion.
 */
void Director::notifyPlayFinished()
{
	this->performingPlay = false;
	this->currentPlay = "";

	const char token = this->terminate ? DIRECTOR_TERMINATED_TOKEN : PLAY_ENDED_TOKEN;

	// Tell the producer that we are done with the current play or that we terminated
	if (stream.send_n(&token, 1) < 0)
	{
		cerr << "Failed to send stop or terminate token" << endl;
	}

	// If we are terminating, end the reactor loop so that the process can end smoothly.
	if (this->terminate && !this->reactorStopped)
	{
		this->reactorStopped = true;
		ACE_Reactor::instance()->end_reactor_event_loop();
	}
}

/*
 * Accessor for the stop state flag.
 */
bool Director::shouldStop()
{
	return this->stop;
}

/*
 * Accessor for the termination state flag.
 */
bool Director::shouldTerminate()
{
	return this->terminate;
}

/*
 * Starts the reactor's event loop so that the director can start
 * responding to producer requests.
 */
void Director::direct()
{
	ACE_Reactor::instance()->run_reactor_event_loop();
}

/*
 * Event handler for socket input. Receives and parses the command
 * from the producer and responds accordingly.
 */
int Director::handle_input(ACE_HANDLE fd)
{
	char token;
	ACE_Time_Value timeout(5);
	while (this->stream.recv_n(&token, 1, &timeout) > 0)
	{
		if (token == START_PLAY_TOKEN)
		{
			if (!this->waitingForPlayName)
			{
				this->waitingForPlayName = true;
			}
			else
			{
				cerr << "Already reading a play name, starting again" << endl;
				this->playNameRead = "";
			}
		}
		else if (token == STOP_PLAY_TOKEN)
		{
			this->stopPlay();
			this->waitingForPlayName = false;
			this->playNameRead = "";
			return 0;
		}
		else if (token == QUIT_TOKEN)
		{
			this->terminateTroupe();
			return 0;
		}
		else if (this->waitingForPlayName)
		{
			if (token != ' ')
			{
				this->playNameRead += token;
			}
			else
			{
				this->cue(this->playNameRead);
				this->playNameRead = "";
				this->waitingForPlayName = false;
				return 0;
			}
		}
		else
		{
			cerr << "Token: " << token << " undefined" << endl;
		}
	}

	return 0;
}

/*
 * Event handler for the end of play notification. Will be called by the current play object
 * once all of the players have exited. This can happen when a play is finished or as part of
 * the early stop/termination protocol. This method will reset the play, notify the producer and
 * end the reactor in the event of a termination signal.
 */
int Director::handle_exception(ACE_HANDLE fd)
{
	if (this->terminate)
	{
		for (shared_ptr<Player> player : this->players)
		{
			player->notifyTerminate();
		}
	}

	if (this->currentPlay != "")
	{
		this->plays[this->currentPlay]->notifyStop();
	}

	if (!this->performingPlay)
	{
		this->notifyPlayFinished();
	}

	return  0;
}

/*
 * Event handler for kill signal. Simply starts the termination protocol.
 */
void handleSignal(int signum)
{
	Director::getInstance()->terminateTroupe();
	signal(SIGINT, handleSignal);
}