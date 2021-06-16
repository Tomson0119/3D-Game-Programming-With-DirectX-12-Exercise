#pragma once

#include "gameObject.h"


class Player : public GameObject
{
public:
	Player(int offset, Mesh* mesh);
	Player(const Player& rhs) = delete;
	Player& operator=(const Player& rhs) = delete;
	virtual ~Player();
};


/////////////////////////////////////////////////////////////////////////////////////
//
class CameraPlayer : public Player
{
public:
	CameraPlayer(int offset, Mesh* mesh, class Camera* camera);
	CameraPlayer(const CameraPlayer& rhs) = delete;
	CameraPlayer& operator=(const CameraPlayer& rhs) = delete;
	virtual ~CameraPlayer();

	virtual class Camera* ChangeCamera(DWORD cameraMode, float elapsedTime);

	virtual void OnPlayerUpdate(float elapsedTime);
	virtual void OnCameraUpdate(float elapsedTime);

private:
	class Camera* mCamera;
};

/////////////////////////////////////////////////////////////////////////////////////
//
class TerrainPlayer : public CameraPlayer
{
public:
	TerrainPlayer(int offset, Mesh* mesh, Camera* camera, TerrainObject* terrain);
	TerrainPlayer(const TerrainPlayer& rhs) = delete;
	TerrainPlayer& operator=(const TerrainPlayer& rhs) = delete;
	virtual ~TerrainPlayer();

};