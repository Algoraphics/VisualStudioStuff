// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// CharacterPart struct and CharacterPartQueue class.
//

#pragma once

#include <map>
#include <fstream>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "DialogueLine.h"
#include "Play.h"

using namespace std;

class Director;

/*
 * Players are active objects that greedily grab CharacterParts
 * from one of the parts queues populated by the Director and perform them. 
 * Players synchronize their behavior using a Play object.
 */
class Player
{
private:
	map<string, shared_ptr<Play>> & plays;
	string playName;
	string characterName;
	map<unsigned, DialogueLine> lines;
	thread t;
	bool inScene;
	mutex mut;
	Director & director;
	condition_variable newPlayReadyCondition;

	// Prevent compiler implementations (do not use)
	Player(Player & other);
	void operator=(const Player & other);
	void operator=(Player && other);

public:
	Player(map<string, shared_ptr<Play>> & plays_, Director& director_);
	Player(Player && other);
	~Player();
	void read(string partFileName);
	void act(unsigned sceneFragment);
	void enter(CharacterPart part);
	void startPlayer();
	void beginPlay(string newPlayName);
	void waitForPlayerTerminate();
	void notifyTerminate();
};