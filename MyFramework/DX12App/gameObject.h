#pragma once

#include "mesh.h"
#include "camera.h"

class GameObject
{
public:
	GameObject(int offset);
	GameObject(int offset, Mesh* mesh);
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	void UpdateConstants(ConstantBuffer<ObjectConstants>* objectCB);

	virtual void Update(float elapsedTime, XMFLOAT4X4* parent);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);

	virtual void UpdateTransform(XMFLOAT4X4* parent);
	void UpdateBoudingBox();

	virtual void SetChild(GameObject* child);

	void SetMesh(Mesh* mesh) { mMesh = mesh; }
	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(XMFLOAT3 pos);
	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void SetLook(XMFLOAT3& look);

public:
	void Move(float dx, float dy, float dz);
	void Move(XMFLOAT3& dir, float dist);

	virtual void Strafe(float dist, bool local=true);
	virtual void Upward(float dist, bool local=true);
	virtual void Walk(float dist, bool local=true);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3& axis, float angle);

	virtual void RotateY(float angle);
	virtual void Pitch(float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(const XMFLOAT3& scale);
	void Scale(float scale);

public:
	XMFLOAT3 GetPosition() const { return mPosition; }
	XMFLOAT3 GetRight() const { return mRight; }
	XMFLOAT3 GetLook() const { return mLook; }
	XMFLOAT3 GetUp() const { return mUp; }
	
	UINT CBIndex() const { return mCBIndex; }
	virtual ObjectConstants GetObjectConstants();
	BoundingOrientedBox GetBoundingBox() const { return mOOBB; }
	
	
protected:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mScaling = { 1.0f, 1.0f, 1.0f };

	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();
	Material mMaterial = {};

	Mesh* mMesh = nullptr;
	UINT mCBIndex = 0;

	BoundingOrientedBox mOOBB = { };

	GameObject* mParent = nullptr;
	GameObject* mChild = nullptr;
	GameObject* mSibling = nullptr;
};