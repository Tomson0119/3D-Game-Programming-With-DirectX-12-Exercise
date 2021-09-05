#pragma once

#include "d3dFramework.h"
#include "gameScene.h"

extern DXGI_SAMPLE_DESC gMsaaStateDesc;

class GameFramework : public D3DFramework
{
public:
	GameFramework();
	GameFramework(UINT width, UINT height);
	GameFramework(const GameFramework& rhs) = delete;
	GameFramework& operator=(const GameFramework& rhs) = delete;
	virtual ~GameFramework();

	virtual bool InitFramework() override;

private:
	virtual void OnResize() override;
	virtual void OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

private:
	std::stack<std::unique_ptr<GameScene>> mScenes;
};