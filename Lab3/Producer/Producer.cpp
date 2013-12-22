#include "stdafx.h"
#include "Producer.h"
#include <signal.h>

shared_ptr<Producer> Producer::instance;

/*
 *	Create the singleton producer instance.
 */
void Producer::createInstance(unsigned short port, const char* ipAddr)
{
	Producer::instance = shared_ptr<Producer>(new Producer(port, ipAddr));
}

/*
 *	Get the singleton producer instance.
 */
Producer *Producer::getInstance()
{
	return Producer::instance.get();
}

Producer::Producer(unsigned short port, const char* ipAddr)
	: acceptor(port, ipAddr, *this), console(*this), terminate(false), nextDirectorId(0)
{
	signal(SIGINT, handleSignal);
}

/*
 *	Start the producer by starting the console thread and running the singleton reactor's
 *	event loop to respond to both user input and director connections.
 */
void Producer::produce()
{
	this->updateUI();
	this->console.startConsole();
	ACE_Reactor::instance()->run_reactor_event_loop();
	this_thread::sleep_for(chrono::milliseconds(100));
}

/*
 *	Add a director to the producer's list of directors and assign it a unique id.
 */
void Producer::addDirector(shared_ptr<Connection> connection)
{
	lock_guard<mutex> guard(this->mut);
	this->directorConnections[this->nextDirectorId] = connection;
	connection->setDirectorId(this->nextDirectorId);
	++this->nextDirectorId;
}

/*
 *	Populate the list of plays available for a given director.
 */
void Producer::setRepertoire(int directorId, vector<string> plays)
{
	lock_guard<mutex> guard(this->mut);
	if (this->directorConnections.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	this->directorRepertoires[directorId] = move(plays);
	this->runningPlays[directorId] = make_pair(-1, false);

	// update UI to have the director's plays
	this->updateUI();
}

/*
 *	Update the producer when a director say it has started a new play.
 *	This function is called when a handle_input gets a PLAY_STARTED_TOKEN.
 */
void Producer::directorStarted(int directorId)
{
	lock_guard<mutex> guard(this->mut);

	// First check that we were actually expecting this director to start a play.
	if (this->directorRepertoires.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	if (this->runningPlays[directorId].first != -1 && this->runningPlays[directorId].second)
	{
		cerr << "Specified director is busy" << endl;
		return;
	}

	if (this->runningPlays[directorId].first == -1)
	{
		cerr << "Specified director was not requested to start" << endl;
		return;
	}

	this->runningPlays[directorId].second = true;

	// update UI to show play running
	this->updateUI();
}

/*
 *	Updates the producer when a director has finished its assigned play and
 *	is ready to perform a new one.
 */
void Producer::directorReady(int directorId)
{
	lock_guard<mutex> guard(this->mut);
	if (this->directorRepertoires.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	this->runningPlays[directorId] = make_pair(-1, false);

	// update UI to make the director available
	// update UI to not have the play running
	this->updateUI();
}

/*
 *	Removes a director from the producer's list of directors. This
 *	should be called when the producer recieved a notification that
 *	a director process is ending.
 */
void Producer::removeDirector(int directorId)
{
	lock_guard<mutex> guard(this->mut);
	if (this->directorRepertoires.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	// Remove the handler for the connection.
	ACE_SOCK_Stream &stream = this->directorConnections[directorId]->getStream();
	if (ACE_Reactor::instance()->remove_handler(stream.get_handle(), READ_MASK) < 0)
	{
		cerr << "Removing connection handler failed" << endl;
	}

	if (stream.close() < 0)
	{
		cerr << "Connection stream failed to close" << endl;
	}

	// Remove the director from our lists.
	this->directorRepertoires.erase(directorId);
	this->runningPlays.erase(directorId);

	// update UI to not have the director's plays
	this->updateUI();

	// If we have been told to terminate and there are no directors
	// left, then end the event loop so the process can exit.
	if (this->terminate && this->directorRepertoires.empty())
	{
		ACE_Reactor::instance()->end_reactor_event_loop();
	}
}

/*
 *	Tell the specified director to start the specified play.
 */
void Producer::startPlay(int directorId, int playId)
{
	lock_guard<mutex> guard(this->mut);
	
	//	First check that we can start the given play.
	if (this->directorRepertoires.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	if (playId < 0 || playId >= static_cast<int>(this->directorRepertoires[directorId].size()))
	{
		cerr << "Specified play does not exist" << endl;
		return;
	}

	if (this->runningPlays[directorId].first != -1)
	{
		cerr << "Specified director is busy" << endl;
		return;
	}

	this->runningPlays[directorId] = make_pair(playId, false);

	ACE_SOCK_Stream & stream = this->directorConnections[directorId]->getStream();
	string playName = this->directorRepertoires[directorId][playId];
	playName += " "; // Add delimiter

	// send start message to director
	if (stream.send_n(&START_PLAY_TOKEN, 1) < 0)
	{
		cerr << "Failed to send start play token to director" << endl;
		return;
	}

	// send play name to director
	if (stream.send_n(playName.c_str(), playName.size()) < 0)
	{
		cerr << "Failed to send play name to director" << endl;
		return;
	}

	// update UI to mark director as unavailable
	this->updateUI();
}

/*
 * Notify the director to stop the play that it's performing.
 */
void Producer::stopPlay(int directorId)
{
	lock_guard<mutex> guard(this->mut);
	if (this->directorRepertoires.count(directorId) == 0)
	{
		cerr << "Specified director does not exist" << endl;
		return;
	}

	if (this->runningPlays[directorId].first == -1)
	{
		cerr << "Specified director is not performing a play" << endl;
		return;
	}

	ACE_SOCK_Stream & stream = this->directorConnections[directorId]->getStream();

	// send stop message to director
	if (stream.send_n(&STOP_PLAY_TOKEN, 1) < 0)
	{
		cerr << "Failed to send stop play token to director" << endl;
	}
}

/*
 *	Starts the termination protocol when a user types quit or CTRL-C.
 *	We first set the terminate flag, then notify all of the directors that
 *	we are responsible for to quit.
 */
void Producer::quit()
{
	unique_lock<mutex> lock(this->mut);
	if (this->terminate) return;
	this->terminate = true;

	// If there are no known directors, just exit right away.
	if (this->directorRepertoires.empty())
	{
		ACE_Reactor::instance()->end_reactor_event_loop();
	}
	else
	{
		// Get a list of living directors.
		vector<int> keys;
		for (auto & pair : this->directorRepertoires)
		{
			keys.push_back(pair.first);
		}

		for (int id : keys)
		{
			ACE_SOCK_Stream & stream = this->directorConnections[id]->getStream();

			// send terminate message to directors
			if (stream.send_n(&QUIT_TOKEN, 1) < 0)
			{
				// If a director cannot be reached, assume that it is already dead.
				cerr << "Failed to send terminate play token to director" << endl;
				lock.unlock();
				this->removeDirector(id);
				lock.lock();
			}
		}
	}
}

/*
 *	Update the user interface that the user will see. This should be called whenever
 *	the producer's state changes.
 */
void Producer::updateUI()
{
	// Clear the terminal window as much as possible.
	cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nList of available directors:\n" << endl;

	// Print the list of directors and their plays.
	for (auto iter : this->directorRepertoires)
	{
		int directorId = iter.first;
		vector<string> & plays = iter.second;

		cout << "Director " << directorId << ": " << (this->runningPlays[directorId].first == -1 ? "" : "(BUSY)") << endl;
		for (size_t i = 0; i < plays.size(); ++i)
		{
			cout << "\tPlay " << i << ": " << plays[i];
			if (this->runningPlays[directorId].first == i && !this->runningPlays[directorId].second)
			{
				cout << " (REQUESTED)";
			}
			else if (this->runningPlays[directorId].first == i && this->runningPlays[directorId].second)
			{
				cout << " (RUNNING)";
			}

			cout << endl;
		}
	}

	if (!this->terminate)
	{
		cout << "\nAvailable commands:\n\t<start> [director id] [play id]\n\t<stop> [director id]\n\t<quit>\nPlease enter a command:" << endl;
	}
	else
	{
		cout << "\nWaiting for directors to terminate..." << endl;
	}
}

/*
 *	Starts the termination protocol. This function is mainly used
 *  as a callback for the signal handler so that we can run the 
 *  termination protocol outside of the signal handler context.
 */
int Producer::handle_exception(ACE_HANDLE fd)
{
	this->quit();
	return  0;
}

/*
 *	Notify the producer to start the termination protocol.
 */
void Producer::notifyQuit()
{
	ACE_Reactor::instance()->notify(this);
}

/*
 *	Accessor for the termination flag.
 */
bool Producer::shouldQuit()
{
	return this->terminate;
}

/*
 * Event handler for kill signal. Simply starts the termination protocol.
 */
void handleSignal(int signum)
{
	Producer::getInstance()->notifyQuit();
	signal(SIGINT, handleSignal);
}