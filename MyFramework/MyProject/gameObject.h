#pragma once

#include "mesh.h"
#include "../MyCommon/constantBuffer.h"

class GameObject
{
public:
	GameObject(int offset, Mesh* mesh);
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	virtual void Update(ConstantBuffer<ObjectConstants>* objectCB);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);

	void UpdateBoudingBox();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 pos);

	void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void Move(float dx, float dy, float dz);
	void MoveStrafe(float dist);
	void MoveUp(float dist);
	void MoveForward(float dist);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3 *axis, float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(float scale);

	void EnableBoundBoxRender(UINT offset, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	
	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	UINT CBIndex() const { return mCBIndex; }
	virtual ObjectConstants GetObjectConstants();

	class BoundBoxObject* GetBoundBoxObject() const;

protected:
	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();
	Material mMaterial = {};

	Mesh* mMesh = nullptr;
	UINT mCBIndex = 0;

	BoundingOrientedBox mOOBB = {};

	class BoundBoxObject* mBBObject = nullptr;
};

class BoundBoxObject : public GameObject
{
public:
	BoundBoxObject(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList, 
		UINT offset, const BoundingOrientedBox& bb);
	BoundBoxObject(const BoundBoxObject& rhs) = delete;
	BoundBoxObject& operator=(const BoundBoxObject& rhs) = delete;
	virtual ~BoundBoxObject();
};

class ColorObject : public GameObject
{
public:
	ColorObject(int offset, Mesh* mesh);
	ColorObject(const ColorObject& rhs) = delete;
	ColorObject& operator=(const ColorObject& rhs) = delete;
	virtual ~ColorObject();
};