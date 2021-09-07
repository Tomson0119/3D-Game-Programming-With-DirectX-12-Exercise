#pragma once

#include "mesh.h"
#include "constantBuffer.h"
#include "camera.h"
#include "texture.h"

class GameObject
{
public:
	GameObject();
	GameObject(const GameObject& rhs) = delete;
	GameObject& operator=(const GameObject& rhs) = delete;
	virtual ~GameObject();

	virtual void Update(float elapsedTime, XMFLOAT4X4* parent);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);
	virtual void UpdateTransform(XMFLOAT4X4* parent);

	void UpdateBoudingBox();

public:
	virtual void SetChild(GameObject* child);
	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(XMFLOAT3 pos);
	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void SetSRVIndex(UINT idx) { mSrvIndex = idx; }
	void SetLook(XMFLOAT3& look);
	void SetMesh(const std::shared_ptr<Mesh>& mesh) { mMesh = mesh; }

public:
	virtual void Strafe(float dist, bool local=true);
	virtual void Upward(float dist, bool local=true);
	virtual void Walk(float dist, bool local=true);

	virtual void RotateY(float angle);
	virtual void Pitch(float angle);

	void Move(float dx, float dy, float dz);
	void Move(XMFLOAT3& dir, float dist);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT3& axis, float angle);

	void Scale(float xScale, float yScale, float zScale);
	void Scale(const XMFLOAT3& scale);
	void Scale(float scale);

public:
	XMFLOAT3 GetPosition() const { return mPosition; }
	XMFLOAT3 GetRight() const { return mRight; }
	XMFLOAT3 GetLook() const { return mLook; }
	XMFLOAT3 GetUp() const { return mUp; }

	UINT GetSRVIndex() const { return mSrvIndex; }
	
	virtual ObjectConstants GetObjectConstants();

	BoundingOrientedBox GetBoundingBox() const { return mOOBB; }	
	
protected:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mScaling = { 1.0f, 1.0f, 1.0f };

	XMFLOAT4X4 mWorld = Matrix4x4::Identity4x4();

	UINT mSrvIndex = 0;
	Material mMaterial = {};
	std::shared_ptr<Mesh> mMesh;

	BoundingOrientedBox mOOBB = { };

	GameObject* mParent = nullptr;
	GameObject* mChild = nullptr;
	GameObject* mSibling = nullptr;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class TerrainObject : public GameObject
{
public:
	TerrainObject();
	TerrainObject(const TerrainObject& rhs) = delete;
	TerrainObject& operator=(const TerrainObject& rhs) = delete;
	virtual ~TerrainObject();

	void BuildTerrainMeshes(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		int width, int depth,
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