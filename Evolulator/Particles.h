#include "stdafx.h"
#include <iostream>
#include <string>
#include <iterator>
#include <boost/circular_buffer.hpp>

#define PATTERNSIZE 8
#define DEFHEALTH 100
#define BOARDHEIGHT 50
#define BOARDWIDTH 100

using namespace std;

enum direction {UP, DOWN, LEFT, RIGHT, STAY};
enum size {SMALL, MED, LARGE, SIZES};
enum rpc {ROCK, PAPER, SCISSORS, RPCS};

/* These classes are declared entirely public, but could not
be declared as structs since structs do not work as well
with templates */
struct point2D {
	unsigned int x;
	unsigned int y;
	point2D()
		:x(0),y(0){}
	point2D(unsigned int _x, unsigned int _y)
		:x(_x),y(_y){}
	friend bool operator< (const point2D & a, const point2D & b);
	friend bool operator== (const point2D & a, const point2D & b);
};

class Particle {
public:
	unsigned int health;
	point2D coords;
	boost::circular_buffer<direction> pattern;
	unsigned int iter;

	Particle()
		: health(0){}
	Particle(point2D inCoords)
		: health(0),coords(inCoords){}
	virtual bool operator> (Particle & a) {return false;}
};

class SizedPart : public Particle {
public:
	size type;

	SizedPart();
	SizedPart(point2D inCoords);
	bool operator> (Particle & a);
};

class RPCPart : public Particle {
public:
	rpc type;

	RPCPart();
	RPCPart(point2D inCoords);
	bool operator> (Particle & a);
};

template <class T>
bool Battle (T & a, T & b) {
	return a > b;
}