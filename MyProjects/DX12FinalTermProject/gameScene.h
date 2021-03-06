#pragma once

#include "gameTimer.h"
#include "camera.h"
#include "constantBuffer.h"

#include "mesh.h"
#include "pipeline.h"
#include "player.h"
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
	void UpdateConstants();
	void Update(const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessMouseDown(HWND hwnd, WPARAM buttonState);
	void OnProcessMouseUp(WPARAM buttonState);
	void OnProcessMouseMove(WPARAM buttonState) { }
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnKeyboardInput(const GameTimer& timer);
	void OnMouseInput(const GameTimer& timer);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device);

	XMFLOAT3 CenterPointScreenToWorld();
	XMFLOAT3 GetCollisionPosWithObjects(XMFLOAT3& start, XMFLOAT3& dir);
	bool OnCollisionWithEnemy(XMFLOAT3& point);
	bool CheckEnemiesDeath();

	void ShowManual();

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
	std::array<EnemyObject*, 16> mEnemies;

	Player* mPlayer = nullptr;
	LineObject* mBulletTrack = nullptr;
	GameObject* mHitIndicator = nullptr;
	TerrainObject* mTerrain = nullptr;
	EnemyObject* mBoss = nullptr;
	GunObject* mGunSlide = nullptr;

	CameraMode mLastCameraMode;
	POINT mLastMousePos;

	float mAspect = 0.0f;
	bool mShowWireFrame = false;
};