#include "stdafx.h"
#include "Particles.h"
#include <iostream>
#include <vector>
#include <map>
#include <set>

#define FOODCHANCE rand() % 10
#define SPAWNCHANCE rand() % 50
#define AGEDAMAGE 5
#define FOODBONUS 15
#define LOSSDAMAGE 20
#define MOVECOST 10
#define MAPTYPE point2D,vector<shared_ptr<Particle>>

using namespace std;

class World {
protected:
	map<MAPTYPE> PartMap;
	set<point2D> Food;
public:
	World();
	void update();
	void makeMoves();
	void move(shared_ptr<Particle> p);
};