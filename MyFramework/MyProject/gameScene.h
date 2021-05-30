#pragma once

#include "../MyCommon/gameTimer.h"
#include "../MyCommon/camera.h"
#include "../MyCommon/constantBuffer.h"

#include "mesh.h"
#include "gameObject.h"
#include "pipeline.h"
#include "shader.h"

struct ObjectConstants
{
	XMFLOAT4X4 World;
};

struct CameraConstants
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Proj;
	XMFLOAT4X4 ViewProj;
};

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void Resize(float aspect);

	void Update(const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::White;

	std::unique_ptr<Camera> mCamera;

	std::unique_ptr<ConstantBuffer<ObjectConstants>> mObjectCB;
	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;

	ComPtr<ID3D12RootSignature> mRootSignature;
	
	std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Pipeline>> mPipelines;
	std::vector<std::unique_ptr<GameObject>> mGameObjects;
};