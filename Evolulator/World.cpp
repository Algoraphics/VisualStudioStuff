#include "stdafx.h"
#include "World.h"

World::World() 
{
	for (unsigned int i = 0; i < BOARDHEIGHT * BOARDWIDTH; ++i) 
	{
		point2D randLoc = point2D(rand() % BOARDWIDTH, rand() % BOARDHEIGHT);

		if (FOODCHANCE == 0) {Food.insert(randLoc);}

		if (SPAWNCHANCE == 0) 
		{
			shared_ptr<SizedPart> shp(new SizedPart(randLoc));
			point2D point = (*shp).coords;
			map<MAPTYPE>::iterator iter = PartMap.find(point);
			if (iter == PartMap.end()) 
			{   //The coordinates are not in the map
				vector<shared_ptr<Particle>> vec; //new vector for this key
				vec.push_back(shp);
				PartMap[point] = vec;
			}
			else 
			{
				iter->second.push_back(shp);
			}
		}
	}
}

void World::update() 
{
	//Loop over each coordinate in the map
	map<MAPTYPE>::iterator it;
	for (it = PartMap.begin(); it != PartMap.end(); it++) 
	{
		unsigned int numParts = it->second.size();
		if (numParts > 1) {//Fight if more than one particle
			//Fighting Loop sets each particle on a space against all others on that space
			for (unsigned int i = 0; i < numParts; ++i) 
			{
				for(unsigned int j = 0; j < numParts; ++j) 
				{
					//Battle, as long as the particle isn't battling itself
					if (i != j) {
						if (Battle(*(it->second[i]),*(it->second[j]))) 
						{
							(*(it->second[j])).health -= LOSSDAMAGE;
						} 
						else 
						{ 
							(*(it->second[i])).health -= LOSSDAMAGE;
						}
					}
				}
			}
		}
		//Once fighting is done, check if there is food here
		if (Food.find(it->first) != Food.end()) 
		{//if so, feed
			for (unsigned int i = 0; i < numParts; ++i) 
			{
				Particle & part = *(it->second[i]);
				if (part.health > 0) 
				{//if the particle is alive
					part.health += FOODBONUS;
				}
				else 
				{//if dead, remove it from the space
					it->second.erase(it->second.begin() + i);
				}
			}
		}
	}
}

void World::makeMoves() 
{
	map<MAPTYPE>::iterator it;
	//Loop over all the spaces that contain particles
	for (it = PartMap.begin(); it != PartMap.end(); it++) 
	{
		//Loop over all the particles on each space
		for (unsigned int i = 0; i < it->second.size(); ++i) 
		{
			shared_ptr<Particle> curPoint = it->second[i];
			//REPRODUCE HERE
			it->second.erase(it->second.begin() + i);
			move(curPoint);
		}
	}
}

void World::move(shared_ptr<Particle> p) 
{
	(*p).health -= MOVECOST;
	direction d = (*p).pattern[(*p).iter]; //Figure out how exactly ++works and use it for return and increment here
	switch (d) 
	{
	case UP:
		(*p).coords.y++;
	case DOWN:
		(*p).coords.y--;
	case LEFT:
		(*p).coords.x--;
	case RIGHT:
		(*p).coords.x++;
	}
}