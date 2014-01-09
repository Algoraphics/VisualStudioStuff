#include "stdafx.h"
#include "Particles.h"

SizedPart::SizedPart() {
	health = DEFHEALTH;
	type = static_cast<size>(rand() % SIZES); //generate a random size
	coords = point2D(rand() % BOARDWIDTH, rand() % BOARDHEIGHT);

	for (int i = 0; i < PATTERNSIZE; i++){ //generate a random pattern for the particle
		pattern.push_back(static_cast<direction>(rand() % STAY));
	}
	iter = 0;
};

SizedPart::SizedPart(point2D inCoords) {
	health = DEFHEALTH;
	type = static_cast<size>(rand() % SIZES); //generate a random size
	coords = inCoords;

	for (int i = 0; i < PATTERNSIZE; i++){ //generate a random pattern for the particle
		pattern.push_back(static_cast<direction>(rand() % STAY));
	}
	iter = 0;
};

RPCPart::RPCPart() {
	health = DEFHEALTH;
	type = static_cast<rpc>(rand() % RPCS); //generate a random RPC
	coords = point2D(rand() % BOARDWIDTH, rand() % BOARDHEIGHT);

	for (int i = 0; i < PATTERNSIZE; i++){ //generate a random pattern for the particle
		pattern.push_back(static_cast<direction>(rand() % STAY));
	}
	iter = 0;
}

RPCPart::RPCPart(point2D inCoords) {
	health = DEFHEALTH;
	type = static_cast<rpc>(rand() % RPCS); //generate a random RPC
	coords = inCoords;

	for (int i = 0; i < PATTERNSIZE; i++) { //generate a random pattern for the particle
		pattern.push_back(static_cast<direction>(rand() % STAY));
	}
	iter = 0;
}

bool operator< (const point2D & a, const point2D & b) {
	if (a.y < b.y) {return true;}
	else if (a.y == b.y) {return a.x < b.x;}
	else return false;
}

bool operator== (const point2D & a, const point2D & b) {
	return (a.x == b.x && a.y == b.y);
}

bool SizedPart::operator> (Particle & a) {
	//Downcast to derived type
	SizedPart * p = dynamic_cast<SizedPart*>(&a);

	return type > p->type;
}

bool RPCPart::operator> (Particle & a) {
	//Downcast to derived type
	RPCPart * p = dynamic_cast<RPCPart*>(&a);

	return (type == ROCK && p->type == SCISSORS ||
			type == PAPER && p->type == ROCK ||
			type == SCISSORS && p->type == PAPER);
}
