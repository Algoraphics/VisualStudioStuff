// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Connection class definition.
//

#include "stdafx.h"
#include "Connection.h"
#include "Producer.h"

using namespace std;

Connection::Connection(Producer & producer_) : 
	receivingPlays(true), 
	producer(producer_), 
	directorId(-1)
{
}

/*
 * Parses an input command from the stream and follows our communication protocal after determining
 * the type of token that has been input. Initially it is assumed that we will receive a play name.
 * Any characters not in our list of special tokens will be concatenated as part of the play name.
 */
int Connection::handle_input(ACE_HANDLE h)
{
	char buffer;
	ACE_Time_Value timeout(5);
	while (this->stream.recv_n(&buffer, 1, &timeout) > 0)
	{
		if (buffer == END_OF_PLAYS_TOKEN)
		{
			if (!this->receivingPlays)
			{
				cerr << "Invalid END_OF_PLAYS_TOKEN" << endl;
				return 0;
			}

			// Add the director's list of plays to the producer
			this->producer.setRepertoire(this->directorId, this->playNames);
			this->playNames.clear();
			receivingPlays = false;
			return 0;
		}
		else if (buffer == PLAY_ENDED_TOKEN)
		{
			if (this->receivingPlays)
			{
				cerr << "Invalid message received while reading play names" << endl;
				return 0;
			}

			// Inform the producer that the director is ready to perform another play
			this->producer.directorReady(this->directorId);
			return 0;
		}
		else if (buffer == PLAY_STARTED_TOKEN)
		{
			if (this->receivingPlays)
			{
				cerr << "Invalid message received while reading play names" << endl;
				return 0;
			}

			// Inform the producer that the director has started the play
			this->producer.directorStarted(this->directorId);
			return 0;
		}
		else if (buffer == DIRECTOR_TERMINATED_TOKEN)
		{
			if (this->receivingPlays)
			{
				cerr << "Invalid message received while reading play names" << endl;
				return 0;
			}

			// Inform the producer that the director has terminated
			this->producer.removeDirector(this->directorId);
			return 0;
		}
		else if (this->receivingPlays)
		{
			if (buffer == ' ')
			{
				this->playNames.push_back(this->curPlayName);
				this->curPlayName = "";
			}
			else
			{
				// Concatenates the current character to the play name we are saving
				this->curPlayName += buffer;
			}
		}
		else
		{
			cerr << "Token: " << buffer << " undefined" << endl;
		}
	}

	// If recv fails, fire the director for incompetence.
	if (this->directorId != -1)
	{
		this->producer.removeDirector(this->directorId);
		this->directorId = -1;
	}

	return 0;
}

/*
 *	Get access to the stream object
 */
ACE_SOCK_Stream& Connection::getStream()
{
	return this->stream;
}

/*
 *	Setter for the director ID
 */
void Connection::setDirectorId(int directorId)
{
	if (this->directorId == -1)
	{
		this->directorId = directorId;
	}
	else
	{
		cerr << "This connection already has a director" << endl;
	}
}