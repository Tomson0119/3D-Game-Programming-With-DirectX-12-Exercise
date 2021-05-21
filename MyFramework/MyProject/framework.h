#pragma once

#include "../MyCommon/d3dFramework.h"
<<<<<<< HEAD
#include "../MyCommon/camera.h"
=======
>>>>>>> master-DESKTOP-BNBSJFG
#include "gameScene.h"

class GameFramework : public D3DFramework
{
public:
	GameFramework();
	GameFramework(const GameFramework& rhs) = delete;
	GameFramework& operator=(const GameFramework& rhs) = delete;
	virtual ~GameFramework();

	virtual bool InitFramework() override;

	void SetFrameSize(UINT width, UINT height) { mFrameWidth = width; mFrameHeight = height; }

private:
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

private:
	std::stack<std::unique_ptr<GameScene>> mScenes;
<<<<<<< HEAD
	std::unique_ptr<Camera> mCamera;
=======


>>>>>>> master-DESKTOP-BNBSJFG
};