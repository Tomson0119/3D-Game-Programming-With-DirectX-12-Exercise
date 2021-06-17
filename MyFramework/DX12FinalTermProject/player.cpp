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

void Player::Walk(float dist, bool updateVelocity)
{
	XMFLOAT3 shift = { 0.0f,0.0f,0.0f };
	shift = Vector3::Add(shift, mCamera->GetLook(), dist);
	Move(shift, updateVelocity);
}

void Player::Strafe(float dist, bool updateVelocity)
{
	XMFLOAT3 shift = { 0.0f,0.0f,0.0f };
	shift = Vector3::Add(shift, mCamera->GetRight(), dist);
	Move(shift, updateVelocity);
}

void Player::Upward(float dist, bool updateVelocity)
{
	XMFLOAT3 shift = { 0.0f,0.0f,0.0f };
	shift = Vector3::Add(shift, XMFLOAT3(0.0f, 1.0f, 0.0f), dist);
	Move(shift, updateVelocity);
}

void Player::Move(XMFLOAT3& shift, bool updateVelocity)
{
	if (updateVelocity)
		mVelocity = Vector3::Add(mVelocity, shift);
	else
	{
		mPosition = Vector3::Add(mPosition, shift);
		mCamera->Move(shift.x, shift.y, shift.z);
	}
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

void Player::Update(float elapsedTime)
{
	mVelocity = Vector3::Add(mVelocity, mGravity);

	// Velocity 값이 너무 커지지 않게 조정한다.
	float length = sqrtf(mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z);
	if (length > mMaxVelocityXZ)
	{
		mVelocity.x *= (mMaxVelocityXZ / length);
		mVelocity.z *= (mMaxVelocityXZ / length);
	}

	length = sqrtf(mVelocity.y * mVelocity.y);
	if (length > mMaxVelocityY)
		mVelocity.y *= (mMaxVelocityY / length);

	// 타이머 시간의 흐름에 맞추어 이동한다.
	XMFLOAT3 velocity = Vector3::ScalarProduct(mVelocity, elapsedTime);
	Move(velocity, false);

	if(mPlayerUpdateContext)
		OnPlayerUpdate(elapsedTime);
	
	if (mCamera->GetMode() == CameraMode::THIRD_PERSON_CAMERA)
		mCamera->Update(elapsedTime);
	if (mCameraUpdateContext)
		OnCameraUpdate(elapsedTime);
	if (mCamera->GetMode() == CameraMode::THIRD_PERSON_CAMERA)
		mCamera->LookAt(mPosition);
	mCamera->UpdateViewMatrix();

	length = Vector3::Length(mVelocity);
	float deceleration = (mFriction * elapsedTime);
	if (deceleration > length) deceleration = length;
	velocity = Vector3::ScalarProduct(mVelocity, -deceleration);
	mVelocity = Vector3::Add(mVelocity, Vector3::Normalize(velocity));

	GameObject::Update(elapsedTime);
}


/////////////////////////////////////////////////////////////////////////////////////
//
TerrainPlayer::TerrainPlayer(int offset, Mesh* mesh, void* context)
	: Player(offset, mesh)
{
	TerrainObject* terrain = (TerrainObject*)context;
	float xPos = terrain->GetWidth() * 0.5f;
	float zPos = terrain->GetDepth() * 0.5f;
	float yPos = terrain->GetHeight(xPos, zPos) + 100.0f;
	SetPosition(xPos, yPos, zPos);

	SetPlayerContext(terrain);
	SetCameraContext(terrain);
}

TerrainPlayer::~TerrainPlayer()
{
}

Camera* TerrainPlayer::ChangeCameraMode(int cameraMode)
{
	if (mCamera && cameraMode == (int)mCamera->GetMode())
		return mCamera;

	mCamera = Player::ChangeCameraMode(cameraMode);

	switch (mCamera->GetMode())
	{
	case CameraMode::FIRST_PERSON_CAMERA:
		mFriction = 2.0f;
		mGravity = { 0.0f, -9.8f, 0.0f };
		mMaxVelocityXZ = 2.5f;
		mMaxVelocityY = 40.0f;

		mCamera->SetOffset(0.0f, 2.0f, 0.0f);
		mCamera->SetTimeLag(0.0f);
		break;

	case CameraMode::THIRD_PERSON_CAMERA:
		mFriction = 20.0f;
		mGravity = { 0.0f, -9.8f, 0.0f };
		mMaxVelocityXZ = 25.5f;
		mMaxVelocityY = 40.0f;

		mCamera->SetOffset(0.0f, 5.0f, -10.0f);
		mCamera->SetTimeLag(0.25f);
		break;

	case CameraMode::TOP_DOWN_CAMERA:
		mCamera->SetOffset(-10.0f, 10.0f, -10.0f);
		break;
	}

	mCamera->SetPosition(Vector3::Add(mPosition, mCamera->GetOffset()));
	mCamera->Update(1.0f);

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
