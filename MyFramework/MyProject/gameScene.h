#pragma once

#include "../MyCommon/gameTimer.h"
#include "pipeline.h"
#include "shader.h"

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildShaders();
	void BuildPSOs(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::Black;

	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Pipeline>> mPipelines;
};