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

	virtual void Update(float elapsedTime, XMFLOAT4X4* parent);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);

	void UpdateTransform();
	void UpdateBoudingBox();

	void SetChild(GameObject* child);

	void SetMesh(Mesh* mesh) { mMeshes.emplace_back(mesh); }
	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(XMFLOAT3 pos);

	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

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

	GameObject* mParent = nullptr;
	GameObject* mChild = nullptr;
	GameObject* mSibling = nullptr;
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