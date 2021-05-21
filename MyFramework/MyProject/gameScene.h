#pragma once

#include "../MyCommon/gameTimer.h"

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Draw(const GameTimer& timer);

	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildShadersAndLayouts();
	void BuildPSOs();

private:
	ComPtr<ID3D12RootSignature> mRootSignature;
};