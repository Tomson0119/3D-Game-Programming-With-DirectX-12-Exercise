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

	void UpdateTransform();
	void UpdateBoudingBox();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 pos);

	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void Move(float dx, float dy, float dz);
	void Move(XMFLOAT3& dir, float dist);

	void MoveStrafe(float dist, bool local=true);
	void MoveUp(float dist, bool local=true);
	void MoveForward(float dist, bool local=true);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3& axis, float angle);

	void RotateY(float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(float scale);

	void EnableBoundBoxRender(UINT offset, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	UINT CBIndex() const { return mCBIndex; }
	virtual ObjectConstants GetObjectConstants();

	class BoundBoxObject* GetBoundBoxObject() const;

protected:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

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

	void UpdateTransform(const XMFLOAT4X4& world) { mWorld = world; }
};

class ColorObject : public GameObject
{
public:
	ColorObject(int offset, Mesh* mesh);
	ColorObject(const ColorObject& rhs) = delete;
	ColorObject& operator=(const ColorObject& rhs) = delete;
	virtual ~ColorObject();
};