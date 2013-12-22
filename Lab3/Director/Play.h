// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Play class.
//

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "DialogueLine.h"
#include "CharacterPartQueue.h"

using namespace std;

class Director;

/* 
 * The play class encapsulates a play that can be performed by a set of players. It stores a list of
 * the scene titles for the play and the corresponding queue of character parts. Play also acts as
 * a synchronization mechanism that Player threads call into when entering a scene, 
 * reciting lines, and leaving a scene to ensure these operations are synchronized with other Players.
 * It also coordinated the stopping of all players when requested.
 */
class Play
{
private:
	static const unsigned lineTimeoutInMillis = 1;
	static const unsigned timeoutTimeoutInMillis = 1000;

	string currentSpeaker;
	mutex mut;
	condition_variable turnToReciteCondition, turnToEnterCondition, timeoutCondition;
	unsigned lineCounter;
	unsigned sceneFragmentCounter;
	unsigned onStage;
	unsigned numPlayersTimedOut;
	bool makingProgress;

	Director & director;
	CharacterPartQueue characterParts;
	vector<string> sceneTitles;
	vector<string>::iterator currentSceneTitle;

	// Prevent compiler implementations (do not use)
	Play(const Play & other);
	void operator=(const Play & other);

public:
	Play(Director & director_);
	void enter(unsigned sceneToEnter);
	void exit();
	void recite(map<unsigned, DialogueLine>::iterator &line, unsigned sceneFragment);
	CharacterPartQueue & getCharacterParts();
	vector<string> & getSceneTitles();
	bool shouldStop();
	void notifyStop();
	void reset();
};