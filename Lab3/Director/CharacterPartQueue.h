// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// CharacterPart struct and CharacterPartQueue class.
//

#pragma once

#include <list>
#include <string>
#include <mutex>

using namespace std;

/*
 * Represents a Character Part, which contains a character name, the scene fragment 
 * in which the part occurs, and a part file containing the lines. A character part
 * is a structure of the appropriate granularity for Players to grab and perform in
 * a greedy manner.
 */
struct CharacterPart
{
	unsigned sceneFragment;
	string characterName, characterPartFile;
	CharacterPart();
	CharacterPart(unsigned sceneFragment_, string characterName_, string characterPartFile_);
};

/*
 * A thread-safe queue that stores CharacterParts. Player threads can grab CharacterParts from
 * this queue in a greedy manner. The Director inserts the CharacterParts in the order they
 * must be assigned to Players.
 */
class CharacterPartQueue
{
private:
	list<CharacterPart> partsQueue;
	list<CharacterPart>::iterator nextPart;
	mutex mut;

public:
	CharacterPartQueue();
	void addPart(CharacterPart part);
	bool retrievePart(CharacterPart & part);
	void reset();
};

