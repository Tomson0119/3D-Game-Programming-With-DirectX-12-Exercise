#include "stdafx.h"
#include "player.h"
#include "camera.h"


Player::Player(int offset, Mesh* mesh)
	: GameObject(offset, mesh)
{

}

Player::~Player()
{
}

Camera* Player::ChangeCameraMode(int cameraMode)
{	
	if (mCamera && (int)mCamera->GetMode() == cameraMode)
		return mCamera;

	Camera* newCamera = nullptr;
	switch (cameraMode)
	{
	case (int)CameraMode::FIRST_PERSON_CAMERA:
		newCamera = new FirstPersonCamera();
		break;

	case (int)CameraMode::THIRD_PERSON_CAMERA:
		newCamera = new ThirdPersonCamera();
		break;

	case (int)CameraMode::TOP_DOWN_CAMERA:
		newCamera = new TopDownCamera();
		break;
	}

	if (newCamera)
	{
		newCamera->SetMode(cameraMode);
		newCamera->SetPlayer(this);
	}
	return newCamera;
}


/////////////////////////////////////////////////////////////////////////////////////
//
TerrainPlayer::TerrainPlayer(int offset, Mesh* mesh, Camera* camera, void* context)
	: Player(offset, mesh)
{
	SetCamera(camera);
	ChangeCameraMode((int)camera->GetMode());

	TerrainObject* terrain = (TerrainObject*)context;
	float xPos = terrain->GetWidth() * 0.5f;
	float zPos = terrain->GetDepth() * 0.5f;
	float yPos = terrain->GetHeight(xPos, zPos);
	SetPosition(xPos, yPos, zPos);

	SetPlayerContext(terrain);
	SetCameraContext(terrain);
}

TerrainPlayer::~TerrainPlayer()
{
}

Camera* TerrainPlayer::ChangeCameraMode(int cameraMode)
{
	if (cameraMode == (int)mCamera->GetMode())
		return mCamera;

	mCamera = Player::ChangeCameraMode(cameraMode);

	switch (mCamera->GetMode())
	{
	case CameraMode::FIRST_PERSON_CAMERA:
		mFriction = 2.0f;
		mGravity = { 0.0f, 0.0f, 0.0f };
		mMaxVelocityXZ = 2.5f;
		mMaxVelocityY = 40.0f;

		mCamera->SetOffset(0.0f, 2.0f, 0.0f);
		mCamera->SetTimeLag(0.0f);
		break;

	case CameraMode::THIRD_PERSON_CAMERA:
		mFriction = 20.0f;
		mGravity = { 0.0f, 0.0f, 0.0f };
		mMaxVelocityXZ = 25.5f;
		mMaxVelocityY = 40.0f;

		mCamera->SetOffset(0.0f, 2.0f, -10.0f);
		mCamera->SetTimeLag(0.25f);
		break;

	case CameraMode::TOP_DOWN_CAMERA:
		mCamera->SetOffset(-10.0f, 10.0f, -10.0f);
		break;
	}

	mCamera->SetPosition(Vector3::Add(mPosition, mCamera->GetOffset()));

	return mCamera;
}

void TerrainPlayer::OnPlayerUpdate(float elapsedTime)
{
	XMFLOAT3 playerPos = GetPosition();
	TerrainObject* terrain = (TerrainObject*)mPlayerUpdateContext;

	float playerHalfHeight = 1.0f;
	float height = terrain->GetHeight(playerPos.x, playerPos.z) + playerHalfHeight;

	if (playerPos.y < height)
	{
		XMFLOAT3 playerVelocity = GetVelocity();
		playerVelocity.y = 0.0f;
		SetVelocity(playerVelocity);
		playerPos.y = height;
		SetPosition(playerPos);
	}
}

void TerrainPlayer::OnCameraUpdate(float elapsedTime)
{
	XMFLOAT3 cameraPos = mCamera->GetPosition();
	TerrainObject* terrain = (TerrainObject*)mCameraUpdateContext;

	float height = terrain->GetHeight(cameraPos.x, cameraPos.z) + 5.0f;

	if (cameraPos.y <= height)
	{
		cameraPos.y = height;
		mCamera->SetPosition(cameraPos);
		
		if (mCamera->GetMode() == CameraMode::THIRD_PERSON_CAMERA)
			mCamera->LookAt(GetPosition());
	}
}
