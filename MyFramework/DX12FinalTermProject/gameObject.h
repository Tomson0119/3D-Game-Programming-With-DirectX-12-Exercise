#pragma once

#include "mesh.h"
#include "constantBuffer.h"

class GameObject
{
public:
	GameObject(int offset, Mesh* mesh);
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	void UpdateConstants(ConstantBuffer<ObjectConstants>* objectCB);

	virtual void Update(float elapsedTime);
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
	void Scale(const XMFLOAT3& scale);
	void Scale(float scale);

	void EnableBoundBoxRender(UINT offset, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	UINT CBIndex() const { return mCBIndex; }
	BoundingOrientedBox OOBB() const { return mOOBB; }
	XMFLOAT3 GetPos() const { return mPosition; }
	virtual ObjectConstants GetObjectConstants();

	class BoundBoxObject* GetBoundBoxObject() const;
	
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

	virtual void Update(float elapsedTime);
	void UpdateCoordinate(const XMFLOAT4X4& world);
};

class NonePlayerObject : public GameObject
{
public:
	NonePlayerObject(int offset, Mesh* mesh);
	NonePlayerObject(const NonePlayerObject& rhs) = delete;
	NonePlayerObject& operator=(const NonePlayerObject& rhs) = delete;
	virtual ~NonePlayerObject();
	
	void SetInitialSpeed(float speed);
	void SetMovingDirection(XMFLOAT3& dir) { mMovingDirection = dir; }
	void SetMovingSpeed(float speed) { mCurrSpeed = speed; }
	void SetAcceleration(float acel) { mAcceleration = acel; }
	void SetActive(bool flag) { mActive = flag; }

	virtual void Update(float elapsedTime);

	bool IsActive() const { return mActive; }
	float GetInitialSpeed() const { return mInitialSpeed; }
	XMFLOAT3 GetMovingDirection() const { return mMovingDirection; }

private:
	XMFLOAT3 mMovingDirection = { };

	bool mActive = true;
	float mInitialSpeed = 0.0f;
	float mCurrSpeed = 0.0f;
	float mAcceleration = 0.1f;
};