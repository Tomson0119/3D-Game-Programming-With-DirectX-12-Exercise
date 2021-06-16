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


/////////////////////////////////////////////////////////////////////////////////////
//
CameraPlayer::CameraPlayer(int offset, Mesh* mesh, Camera* camera)
	: Player(offset, mesh), mCamera(camera)
{
}

CameraPlayer::~CameraPlayer()
{
}

Camera* CameraPlayer::ChangeCamera(DWORD cameraMode, float elapsedTime)
{
	return nullptr;
}

void CameraPlayer::OnPlayerUpdate(float elapsedTime)
{
}

void CameraPlayer::OnCameraUpdate(float elapsedTime)
{
}


/////////////////////////////////////////////////////////////////////////////////////
//
TerrainPlayer::TerrainPlayer(int offset, Mesh* mesh, Camera* camera, TerrainObject* terrain)
	: CameraPlayer(offset, mesh, camera)
{
	float xPos = terrain->GetWidth() * 0.5f;
	float zPos = terrain->GetDepth() * 0.5f;
	float yPos = terrain->GetHeight(xPos, zPos);
	SetPosition(xPos, yPos, zPos);
}

TerrainPlayer::~TerrainPlayer()
{
}
