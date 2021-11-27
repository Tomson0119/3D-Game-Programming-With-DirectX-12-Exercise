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

	void OnResize(float aspect);

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, float aspect);
	void UpdateCameraConstant(int idx, Camera* camera);
	void UpdateConstants();
	void Update(ID3D12Device* device, const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex = 0);

	void PrepareCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void OnProcessMouseDown(HWND hwnd, WPARAM buttonState, int x, int y);
	void OnProcessMouseUp(WPARAM buttonState, int x, int y);
	void OnProcessMouseMove(WPARAM buttonState, int x, int y);
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnPreciseKeyInput(float elapsed);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }
	ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildDescriptorHeap(ID3D12Device* device);

	void BuildRoomObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void CollisionProcess(ID3D12Device* device);
	void CreateAndAppendDustBillboard(ID3D12Device* device);
	void CreateAndAppendFlameBillboard(ID3D12Device* device, GameObject* box);
	void DeleteTimeOverBillboards(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::LightSkyBlue;

	Camera* mCurrentCamera = nullptr;
	std::unique_ptr<Camera> mMainCamera;
	std::unique_ptr<Camera> mPlayerCamera;
	POINT mLastMousePos{};

	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;
	std::unique_ptr<ConstantBuffer<LightConstants>> mLightCB;
	std::unique_ptr<ConstantBuffer<GameInfoConstants>> mGameInfoCB;

	ComPtr<ID3D12RootSignature> mRootSignature;
	
	std::unique_ptr<DynamicCubeRenderer> mCubeMapRenderer;

	std::map<Layer, std::unique_ptr<Pipeline>> mPipelines;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	
	Player* mPlayer = nullptr;
	GameObject* mReflectedPlayer = nullptr;

	std::shared_ptr<Billboard> mFlameBillboard;
	std::shared_ptr<Billboard> mDustBillboard;
	std::chrono::high_resolution_clock::time_point mPrevTime;

	bool mLODSet = false;
	bool mOutside = true;
};