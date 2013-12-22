// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Player class definition.
//

#include "stdafx.h"
#include "Player.h"
#include "utils.h"
#include "Director.h"
#include <sstream>

Player::Player(map<string, shared_ptr<Play>> & plays_, Director& director_) : plays(plays_), director(director_), inScene(false)
{
}

Player::Player(Player && other) :
	plays(other.plays),
	playName(move(other.playName)),
	characterName(move(other.characterName)),
	lines(other.lines),
	t(move(other.t)),
	inScene(other.inScene),
	director(other.director)
{
}

Player::~Player()
{
	this->waitForPlayerTerminate();
}

/*
 * Read the character's lines for the current scene fragment from a file.
 */
void Player::read(string partFileName)
{
	// Now that we're reading a new part, clear all of our previous lines
	this->lines.clear();

	// Open the part file
	ifstream partFile(partFileName);
	if (!partFile.is_open())
	{
		string message = "Could not open file ";
		cerr << message << partFileName << endl;
		throw logic_error(message);
	}

	string fileLine;
	while (getline(partFile, fileLine))
	{
		istringstream fileLineStream(fileLine);
		unsigned playLineNum;
		string playLine;

		// Verify that the line contains a line number and non-whitespace text
		if(fileLineStream >> playLineNum && getline(fileLineStream, playLine) && trim(playLine) != "")
		{
			DialogueLine playLineStruct(playLineNum, this->characterName, playLine);
			this->lines.insert(make_pair(playLineNum, playLineStruct));
		}
	}
}

/*
 * Recite all of the character's lines for the current scene fragment synchronously with the other players.
 */
void Player::act(unsigned sceneFragment)
{
	this->plays[this->playName]->enter(sceneFragment);
	this->inScene = true;

	auto line = this->lines.begin();
	while (line != this->lines.end() && !this->plays[playName]->shouldStop())
	{
		this->plays[this->playName]->recite(line, sceneFragment);
	}

	this->inScene = false;
	this->plays[this->playName]->exit();
}

/*
 * Enter a new scene fragment as the specified character.
 */
void Player::enter(CharacterPart part)
{
	this->characterName = part.characterName;
	this->read(part.characterPartFile);
	this->act(part.sceneFragment); 
}

/*
 * Run this player in a separate thread.
 */
void Player::startPlayer()
{
	// Don't start a player more than once
	if (this->t.joinable())
	{
		return;
	}

	// Start the player's thread
	this->t = thread([this]
	{ 
		// This lock controls access to playName
		unique_lock<mutex> lock(this->mut);
		try
		{
			while (!this->director.shouldTerminate())
			{
				// Wait for the director to assign a play
				this->newPlayReadyCondition.wait(lock, [this]{ return this->playName != "" || this->director.shouldTerminate(); });

				// Greedily grab and perform parts for the current play
				CharacterPart part;
				while (!this->director.shouldStop() && this->plays[this->playName]->getCharacterParts().retrievePart(part))
				{
					this->enter(part);
				}

				// Indicate that no play is assigned to the player
				this->playName = "";
			}
		}
		catch (...)
		{
			// If a player has a heart attack during a scene, the player should make
			// sure to exit the stage before dying so as to not deadlock the performance.
			if (this->inScene)
			{
				this->plays[this->playName]->exit();
			}

			return;
		}
	});
}

/*
 * Tell the player to begin performing a new play.
 */
void Player::beginPlay(string newPlayName)
{
	// Blocks until the player is waiting for a new play.
	unique_lock<mutex> lock(this->mut);
	this->playName = newPlayName;
	this->newPlayReadyCondition.notify_all();
}

/*
 * Wait for this player thread to end.
 */
void Player::waitForPlayerTerminate()
{
	if (this->t.joinable())
	{
		this->t.join();
	}
}

/*
 *	Notify the waiting player thread that we are executing the 
 *	termination protocol.
 */
void Player::notifyTerminate()
{
	unique_lock<mutex> lock(this->mut, defer_lock);
	if (lock.try_lock())
	{
		this->newPlayReadyCondition.notify_all();
	}
}