#pragma once

#include "gameObject.h"
#include "camera.h"


class Player : public GameObject
{
public:
	Player();
	Player(const Player& rhs) = delete;
	Player& operator=(const Player& rhs) = delete;
	virtual ~Player();

public:
	void Walk(float dist, bool updateVelocity=true);
	void Strafe(float dist, bool updateVelocity=true);
	void Upward(float dist, bool updateVelocity=true);

	void Move(XMFLOAT3& shift, bool updateVelocity);

	virtual void RotateY(float angle) override;
	virtual void Pitch(float angle) override;

	void SetCamera(Camera* camera) { mCamera = camera; }
	void SetPlayerContext(void* context) { mPlayerUpdateContext = context; }
	void SetCameraContext(void* context) { mCameraUpdateContext = context; }

	void SetVelocity(const XMFLOAT3& vel) { mVelocity = vel; }
	void SetGravity(const XMFLOAT3& grav) { mGravity = grav; }

	XMFLOAT3 GetVelocity() const { return mVelocity; }
	XMFLOAT3 GetGravity() const { return mGravity; }

public:
	virtual Camera* ChangeCameraMode(int cameraMode);

	virtual void Update(float elapsedTime, XMFLOAT4X4* parent) override;
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
	TerrainPlayer(void* context);
	TerrainPlayer(const TerrainPlayer& rhs) = delete;
	TerrainPlayer& operator=(const TerrainPlayer& rhs) = delete;
	virtual ~TerrainPlayer();

	virtual Camera* ChangeCameraMode(int cameraMode) override;

	virtual void OnPlayerUpdate(float elapsedTime) override;
	virtual void OnCameraUpdate(float elapsedTime) override;
};