// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Play class definition.
//

#include "stdafx.h"
#include "Play.h"
#include "Director.h"
#include <chrono>

using namespace chrono;

Play::Play(Director & director_) :
	currentSpeaker(""),
	lineCounter(1),
	sceneFragmentCounter(1),
	onStage(0),
	numPlayersTimedOut(0),
	makingProgress(false),
	director(director_)
{
}

/*
 * Increases the number of active players. Players that try to enter early
 * wait for their scene to start before becoming active.
 */
void Play::enter(unsigned sceneToEnter)
{
	unique_lock<mutex> lock(mut);
	this->director.notifyPlayStarted();

	if (this->sceneFragmentCounter > sceneToEnter)
	{
		string message = "Player missed his/her entire scene. Stupid player...";
		cerr << message << endl;
		throw logic_error(message);
	}
	
	this->turnToEnterCondition.wait(lock, [this, sceneToEnter] { return this->sceneFragmentCounter == sceneToEnter || this->shouldStop(); });

	// Print scene title
	if (this->onStage == 0 && !this->sceneTitles.empty() && !this->shouldStop())
	{
		if (!this->currentSceneTitle->empty())
		{
			cout << "\n\n" << *this->currentSceneTitle << endl;
		}
		else
		{
			cout << "[CHARACTER TRANSITION]" << endl;
		}

		++this->currentSceneTitle;
	}

	++this->onStage;
}

/*
 * Decreases the number of active players. When there are no more active players, move
 * on to the next scene. If the play is finished or we are told to stop, notify the director.
 */
void Play::exit()
{
	lock_guard<mutex> guard(mut);

	--this->onStage;
	if (this->onStage == 0)
	{
		if (this->currentSceneTitle == this->sceneTitles.end() || this->director.shouldStop())
		{
			// When the last scene ends, notify the director that the play is over.
			this->director.notifyPlayFinished();
		}
		else
		{
			// Otherwise prepare for the next scene.
			this->lineCounter = 1;
			++this->sceneFragmentCounter;
			this->turnToEnterCondition.notify_all();
		}
	}
	else if (this->onStage < 0)
	{
		throw logic_error("There are no players left to exit.");
	}
}

/*
 * Recite the provided line at the appropriate time.
 */
void Play::recite(map<unsigned, DialogueLine>::iterator &line, unsigned sceneFragment)
{
	unique_lock<mutex> lock(mut);

	if (this->onStage == 0)
	{
		string message = "There must be at least 1 active player when a line is recited.";
		cerr << message << endl;
		throw logic_error(message);
	}

	// Wait until it is my turn, or someone took my turn, or someone forgot their turn (deadlock avoidance).
	this->turnToReciteCondition.wait_for(lock, milliseconds(this->lineTimeoutInMillis), [this, &line, &sceneFragment]
	{ 
		return this->shouldStop() || this->sceneFragmentCounter > sceneFragment || 
			(this->sceneFragmentCounter == sceneFragment && this->lineCounter >= line->first);
	});

	// If the play is stopping, don't recite anything.
	if (this->shouldStop())
	{
		return;
	}
	
    // If it is my turn, read my line.
	if (this->sceneFragmentCounter == sceneFragment && this->lineCounter == line->first)
	{
		if (currentSpeaker != line->second.speaker)
		{
			currentSpeaker = line->second.speaker;
			cout << endl << currentSpeaker << "." << endl;
		}

		cout << line->second.text << endl;
		++this->lineCounter;
		++line;

		this->makingProgress = true;
	}
	// If my line number was already read by someone else, skip my line.
	else if (this->sceneFragmentCounter > sceneFragment ||
		(this->sceneFragmentCounter == sceneFragment && this->lineCounter > line->first))
	{
		cerr << "Line number " << line->first << " in scene fragment " << sceneFragment << " was already recited." << endl;
		++line;

		this->makingProgress = true;
	}
	// If no one reads a line before a timeout, skip the line.
	else
	{
		// The first thread to timeout reports that it doesn't think that progress is being made.
		// (It is possible that other threads are simply taking a long time to recite all their lines).
		if (this->numPlayersTimedOut == 0)
		{
			this->makingProgress = false;
		}

		// Increment the number of threads that have timed out.
		++this->numPlayersTimedOut;
		this->timeoutCondition.notify_all();

		// Wait until either all threads agree that there is a timeout (a line is missing) or another thread reports that
		// it made progress.
		this->timeoutCondition.wait_for(lock, milliseconds(this->timeoutTimeoutInMillis), [this]{ 
			return this->shouldStop() || this->makingProgress || this->numPlayersTimedOut == this->onStage; 
		});

		if (this->shouldStop())
		{
			return;
		}

		// In the event of a timeout, the first thread through the condition should report the missing line and skip the line.
		if (this->numPlayersTimedOut == this->onStage)
		{
			cerr << "Someone forgot line number " << this->lineCounter << " in scene fragment " << sceneFragmentCounter << "." << endl;
			++this->lineCounter;
			this->makingProgress = true;
		}

		--this->numPlayersTimedOut;

		// Make sure that the timeout correction has completed before any thread continues to recite.
		// (This prevents wraparound bugs).
		this->timeoutCondition.wait_for(lock, milliseconds(this->timeoutTimeoutInMillis), [this]{ return this->shouldStop() || this->numPlayersTimedOut == 0; });
		if (this->shouldStop())
		{
			return;
		}

	}

	this->turnToReciteCondition.notify_all();
	this->timeoutCondition.notify_all();
}

/*
 * Accessor for the queue of character parts.
 */
CharacterPartQueue & Play::getCharacterParts()
{
	return this->characterParts;
}

/*
 * Accessor for the list of scene titles.
 */
vector<string> & Play::getSceneTitles()
{
	return this->sceneTitles;
}

/*
 * Pass through accessor for the stop flag.
 */
bool Play::shouldStop()
{
	return this->director.shouldStop();
}

/*
 * Notify all of the waiting players in the event of a stop signal.
 */
void Play::notifyStop()
{
	lock_guard<mutex> lock(this->mut);
	this->timeoutCondition.notify_all();
	this->turnToEnterCondition.notify_all();
	this->turnToReciteCondition.notify_all();
}

/*
 *	Reset the play object, so that the play can be performed
 *	again from the beginning.
 */
void Play::reset()
{
	lock_guard<mutex> lock(this->mut);
	this->currentSpeaker = "";
	this->lineCounter = 1;
	this->sceneFragmentCounter = 1;
	this->onStage = 0;
	this->numPlayersTimedOut = 0;
	this->makingProgress = false;
	this->currentSceneTitle = this->sceneTitles.begin();
	this->characterParts.reset();
}