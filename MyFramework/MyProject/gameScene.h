#pragma once

#include "../MyCommon/gameTimer.h"
#include "../MyCommon/camera.h"
#include "../MyCommon/constantBuffer.h"

#include "pipeline.h"
#include "shader.h"


struct CommonConstants
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Proj;
};

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void Update(const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildGameObjects();
	void BuildShaders();
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildPSOs(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::Black;

	std::unique_ptr<Camera> mCamera;
	std::unique_ptr<ConstantBuffer<CommonConstants>> mCommonCB;

	ComPtr<ID3D12RootSignature> mRootSignature;	
	
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Pipeline>> mPipelines;
};