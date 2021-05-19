#pragma once

#include "../MyCommon/gameTimer.h"

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects();
	void Draw(const GameTimer& timer);

	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
};