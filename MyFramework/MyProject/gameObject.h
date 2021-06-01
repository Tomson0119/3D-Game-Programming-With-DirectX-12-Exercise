#pragma once

#include "mesh.h"


class GameObject
{
public:
	GameObject(int offset, Mesh* mesh);
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	virtual void Update();
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 pos);

	void Move(float dx, float dy, float dz);
	void MoveStrafe(float dist);
	void MoveUp(float dist);
	void MoveForward(float dist);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3 *axis, float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(float scale);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	UINT CBIndex() const { return mCBIndex; }
	virtual ObjectConstants GetObjectConstants() = 0;

protected:
	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();

	Mesh* mMesh = nullptr;
	UINT mCBIndex = 0;
};

class ColorObject : public GameObject
{
public:
	ColorObject(int offset, Mesh* mesh);
	ColorObject(const ColorObject& rhs) = delete;
	ColorObject& operator=(const ColorObject& rhs) = delete;
	virtual ~ColorObject();

	virtual void Update() override;
	virtual ObjectConstants GetObjectConstants() override;

	void SetColor(XMFLOAT4 color);
	void SetMaterial(XMFLOAT4 Color, XMFLOAT3 Frenel, float Roughness);

private:
	Material mMaterial = {};

};