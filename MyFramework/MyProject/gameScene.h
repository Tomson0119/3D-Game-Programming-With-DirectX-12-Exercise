#pragma once

#include "../MyCommon/gameTimer.h"
#include "../MyCommon/camera.h"
#include "../MyCommon/constantBuffer.h"

#include "mesh.h"
#include "gameObject.h"
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

	void Resize(float aspect);
	void Update(const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessMouseDown(HWND hwnd, WPARAM buttonState, int x, int y);
	void OnProcessMouseUp(WPARAM buttonState, int x, int y);
	void OnProcessMouseMove(WPARAM buttonState, int x, int y);
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) { }

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::LightSkyBlue;

	std::unique_ptr<Camera> mCamera;

	std::unique_ptr<ConstantBuffer<ObjectConstants>> mObjectCB;
	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;
	std::unique_ptr<ConstantBuffer<LightConstants>> mLightCB;

	ComPtr<ID3D12RootSignature> mRootSignature;
	
	std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Pipeline>> mPipelines;
	std::vector<std::unique_ptr<GameObject>> mGameObjects;

	Light mSun;

	POINT mLastMousePos;
};