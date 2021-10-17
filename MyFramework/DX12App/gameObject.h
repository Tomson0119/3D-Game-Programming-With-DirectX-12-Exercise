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
	TerrainObject(int width, int depth, const XMFLOAT3& scale = { 1.0f,1.0f,1.0f });
	TerrainObject(const TerrainObject& rhs) = delete;
	TerrainObject& operator=(const TerrainObject& rhs) = delete;
	virtual ~TerrainObject();

	void BuildHeightMap(const std::wstring& path);
	void BuildTerrainMesh(ID3D12Device* device,	ID3D12GraphicsCommandList* cmdList);

public:
	float GetHeight(float x, float z) const;
	XMFLOAT3 GetNormal(float x, float z) const;

	int GetHeightMapWidth() const { return mHeightMapImage->GetWidth(); }
	int GetHeightMapDepth() const { return mHeightMapImage->GetDepth(); }
	
	void SetScale(const XMFLOAT3& scale) { mTerrainScale = scale; }
	XMFLOAT3 GetTerrainScale() const { return mTerrainScale; }

	float GetWidth() const { return mWidth * mTerrainScale.x; }
	float GetDepth() const { return mDepth * mTerrainScale.z; }

private:
	std::unique_ptr<HeightMapImage> mHeightMapImage;

	int mWidth = 0;
	int mDepth = 0;

	XMFLOAT3 mTerrainScale = { 1.0f,1.0f,1.0f };
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class Billboard : public GameObject
{
public:
	struct BillboardVertex
	{
		XMFLOAT3 Position;
		XMFLOAT2 Size;
	};

public:
	Billboard(float width, float height);
	virtual ~Billboard();

	void AppendBillboard(const XMFLOAT3& pos);
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void UpdateLook(Camera* camera);

private:
	float mWidth = 0.0f;
	float mHeight = 0.0f;

	std::vector<BillboardVertex> mVertices;
	std::vector<UINT> mIndices;
};