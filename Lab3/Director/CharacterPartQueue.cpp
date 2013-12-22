// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// CharacterPart struct and CharacterPartQueue class definitions.
//

#include "stdafx.h"
#include "CharacterPartQueue.h"

CharacterPart::CharacterPart() : sceneFragment(0), characterName(""), characterPartFile("")
{
}

CharacterPart::CharacterPart(unsigned sceneFragment_, string characterName_, string characterPartFile_) : 
	sceneFragment(sceneFragment_), characterName(characterName_), characterPartFile(characterPartFile_)
{
}

CharacterPartQueue::CharacterPartQueue() : nextPart(this->partsQueue.begin())
{
}

/*
 * Thread-safe method used by the director to add parts to the queue.
 */
void CharacterPartQueue::addPart(CharacterPart part)
{
	lock_guard<mutex> guard(this->mut);
	this->partsQueue.push_back(part);
}

/*
 * Thread-safe method used by the Players to greedily retrieve parts from the queue.
 * Returns false if the queue is empty. Otherwise returns true and sets the part
 * reference to the first part in the queue and "removes" the part from the queue.
 * (Parts are not actually discarded as the Director may perform the same play several times.)
 */
bool CharacterPartQueue::retrievePart(CharacterPart & part)
{
	lock_guard<mutex> guard(this->mut);
	if (this->nextPart == this->partsQueue.end())
	{
		return false;
	}
	else
	{
		part = *this->nextPart;
		++this->nextPart;
		return true;
	}
}

/*
 * "Repopulate" the queue by reseting the head iterator to point to the begining of the stored data.
 */
void CharacterPartQueue::reset()
{
	this->nextPart = this->partsQueue.begin();
}
