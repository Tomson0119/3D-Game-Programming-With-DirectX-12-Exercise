#pragma once

#include "mesh.h"
#include "constantBuffer.h"

class GameObject
{
public:
	GameObject(int offset);
	GameObject(int offset, Mesh* mesh);
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	void UpdateConstants(ConstantBuffer<ObjectConstants>* objectCB);

	virtual void Update(float elapsedTime);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);

	void UpdateTransform();
	void UpdateBoudingBox();

	void SetMesh(Mesh* mesh) { mMeshes.emplace_back(mesh); }
	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 pos);

	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void Move(float dx, float dy, float dz);
	void Move(XMFLOAT3& dir, float dist);

	void Strafe(float dist, bool local=true);
	void Upward(float dist, bool local=true);
	void Walk(float dist, bool local=true);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3& axis, float angle);

	void RotateY(float angle, bool local=true);
	void Pitch(float angle, bool local=true);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(const XMFLOAT3& scale);
	void Scale(float scale);

	UINT CBIndex() const { return mCBIndex; }
	BoundingOrientedBox OOBB() const { return mOOBB; }
	XMFLOAT3 GetPos() const { return mPosition; }
	virtual ObjectConstants GetObjectConstants();
	
protected:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mScaling = { 1.0f, 1.0f, 1.0f };	

	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();
	Material mMaterial = {};

	std::vector<Mesh*> mMeshes;
	UINT mCBIndex = 0;

	BoundingOrientedBox mOOBB = {};
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class TerrainObject : public GameObject
{
public:
	TerrainObject(int offset);
	TerrainObject(const TerrainObject& rhs) = delete;
	TerrainObject& operator=(const TerrainObject& rhs) = delete;
	virtual ~TerrainObject();

	void BuildTerrainMeshes(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		int width, int depth, int blockWidth, int blockDepth,
		const XMFLOAT3& scale,
		XMFLOAT4& color,
		const std::wstring& path);

public:
	float GetHeight(float x, float z) const;
	XMFLOAT3 GetNormal(float x, float z) const;

	int GetHeightMapWidth() const { return mHeightMapImage->GetWidth(); }
	int GetHeightMapDepth() const { return mHeightMapImage->GetDepth(); }

	XMFLOAT3 GetScale() const { return mScale; }

	float GetWidth() const { return mWidth * mScale.x; }
	float GetDepth() const { return mDepth * mScale.z; }

private:
	std::unique_ptr<HeightMapImage> mHeightMapImage;
	XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };
	int mWidth = 0;
	int mDepth = 0;
};