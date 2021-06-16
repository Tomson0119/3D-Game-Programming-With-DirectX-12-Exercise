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

void Player::SetPosition(float x, float y, float z)
{
	GameObject::SetPosition(x, y, z);
	if (mCamera) {
		XMFLOAT3 cameraOffset = mCamera->GetOffset();
		mCamera->SetPosition(x + cameraOffset.x, y + cameraOffset.y, z + cameraOffset.z);
	}
}

void Player::SetPosition(XMFLOAT3 pos)
{
	SetPosition(pos.x, pos.y, pos.z);
}

void Player::Strafe(float dist, bool local)
{
	GameObject::Strafe(dist, local);
	//if (mCamera) mCamera->Strafe(dist);
}

void Player::Upward(float dist, bool local)
{
	GameObject::Upward(dist, local);
	//if (mCamera) mCamera->Upward(dist);
}

void Player::Walk(float dist, bool local)
{
	GameObject::Walk(dist, local);
	//if (mCamera) mCamera->Walk(dist);
}

void Player::RotateY(float angle)
{
	GameObject::RotateY(angle);
}

void Player::Pitch(float angle, bool local)
{	
	GameObject::Pitch(angle);
}

void Player::ChangeCameraMode(int cameraMode)
{
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

void TerrainPlayer::ChangeCameraMode(int cameraMode)
{
	if (!mCamera) return;
	mCamera->ChangeMode(cameraMode);
	switch (mCamera->GetMode())
	{
	case CameraMode::FIRST_PERSON_CAMERA:
		mCamera->SetOffset(0.0f, 2.0f, 0.0f);
		break;

	case CameraMode::THIRD_PERSON_CAMERA:
		mCamera->SetOffset(0.0f, 2.0f, -10.0f);
		break;

	case CameraMode::TOP_DOWN_CAMERA:
		mCamera->SetOffset(-10.0f, 10.0f, -10.0f);
		break;
	}
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
