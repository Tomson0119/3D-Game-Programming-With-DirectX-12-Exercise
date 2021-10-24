#pragma once

#include "gameTimer.h"
#include "camera.h"
#include "constantBuffer.h"

#include "mesh.h"
#include "pipeline.h"
#include "player.h"
#include "shader.h"
#include "texture.h"

class DynamicCubeRenderer;

class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void UpdateCameraConstant(int idx, Camera* camera);
	void UpdateConstants(Camera* camera);
	void Update(const GameTimer& timer, Camera* camera);
	void Draw(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex = 0, bool predraw = false);

	void PrepareCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void OnProcessMouseDown(WPARAM buttonState, int x, int y) {}
	void OnProcessMouseUp(WPARAM buttonState, int x, int y) {}
	void OnProcessMouseMove(WPARAM buttonState) {}
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) { }

	void OnPreciseKeyInput(const GameTimer& timer) { }

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }
	ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildDescriptorHeap(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::LightSkyBlue;

	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;
	std::unique_ptr<ConstantBuffer<LightConstants>> mLightCB;

	ComPtr<ID3D12RootSignature> mRootSignature;
	
	std::unique_ptr<DynamicCubeRenderer> mCubeMapRenderer;

	std::unordered_map<Layer, std::unique_ptr<Pipeline>> mPipelines;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	
	GameObject* mPlayer = nullptr;

	std::vector<Billboard*> mBillboards;
};