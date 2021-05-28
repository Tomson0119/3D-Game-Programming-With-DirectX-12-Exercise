#pragma once

#include "../MyCommon/mesh.h"

class GameObject // RenderItem
{
public:
	GameObject();
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	virtual void Update();
	virtual void Draw();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 pos);

	void Move(float dx, float dy, float dz);
	void MoveStrafe(float dist);
	void MoveUp(float dist);
	void MoveForward(float fdist);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(XMFLOAT3 axis, float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(float scale);

protected:
	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();
	Mesh* mMesh = nullptr;
};

class ColoredObject : public GameObject
{
public:
	ColoredObject();
	ColoredObject(const GameObject& rhs) = delete;
	ColoredObject& operator=(const ColoredObject& rhs) = delete;
	virtual ~ColoredObject();

	void SetColor(XMFLOAT4 color);
};