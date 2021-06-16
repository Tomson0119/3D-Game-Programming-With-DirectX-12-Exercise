#pragma once

#include "gameObject.h"
#include "camera.h"

class Player : public GameObject
{
public:
	Player(int offset, Mesh* mesh);
	Player(const Player& rhs) = delete;
	Player& operator=(const Player& rhs) = delete;
	virtual ~Player();

	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(XMFLOAT3 pos);

	virtual void Strafe(float dist, bool local=true);
	virtual void Upward(float dist, bool local=true);
	virtual void Walk(float dist, bool local=true);

	virtual void RotateY(float angle);
	virtual void Pitch(float angle, bool local=true);

public:
	void SetCamera(Camera* camera) { mCamera = camera; mCamera->SetPlayer(this); }
	void SetPlayerContext(void* context) { mPlayerUpdateContext = context; }
	void SetCameraContext(void* context) { mCameraUpdateContext = context; }

	void SetVelocity(const XMFLOAT3& vel) { mVelocity = vel; }
	void SetGravity(const XMFLOAT3& grav) { mGravity = grav; }

	XMFLOAT3 GetVelocity() const { return mVelocity; }
	XMFLOAT3 GetGravity() const { return mGravity; }

public:
	virtual void ChangeCameraMode(int cameraMode);
	virtual void OnPlayerUpdate(float elapsedTime) { }
	virtual void OnCameraUpdate(float elapsedTime) { }

protected:
	XMFLOAT3 mVelocity = {};
	XMFLOAT3 mGravity = {};
	
	float mMaxVelocityXZ = 0.0f;
	float mMaxVelocityY = 0.0f;
	float mFriction = 0.0f;

	void* mPlayerUpdateContext = nullptr;
	void* mCameraUpdateContext = nullptr;

	Camera* mCamera = nullptr;
};


/////////////////////////////////////////////////////////////////////////////////////
//
class TerrainPlayer : public Player
{
public:
	TerrainPlayer(int offset, Mesh* mesh, Camera* camera, void* context);
	TerrainPlayer(const TerrainPlayer& rhs) = delete;
	TerrainPlayer& operator=(const TerrainPlayer& rhs) = delete;
	virtual ~TerrainPlayer();

	virtual void ChangeCameraMode(int cameraMode) override;

	virtual void OnPlayerUpdate(float elapsedTime) override;
	virtual void OnCameraUpdate(float elapsedTime) override;
};