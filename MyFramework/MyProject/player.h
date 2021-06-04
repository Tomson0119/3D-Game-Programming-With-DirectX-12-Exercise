#pragma once

#include "gameObject.h"

class Player : public GameObject
{
public:
	Player(int offset, Mesh* mesh);
	Player(const Player& rhs) = delete;
	Player& operator=(const Player& rhs) = delete;
	virtual ~Player();

private:

};