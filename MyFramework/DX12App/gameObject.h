#pragma once

#include "mesh.h"
#include "constantBuffer.h"
#include "camera.h"
#include "texture.h"

class GameScene;

class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Update(float elapsedTime, XMFLOAT4X4* parent);
	virtual void ExecuteSO(ID3D12GraphicsCommandList* cmdList);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList);
	virtual void UpdateTransform(XMFLOAT4X4* parent);

	void UpdateBoudingBox();
	void Animate(float elapsedTime);

public:
	virtual void SetChild(GameObject* child);
	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(const XMFLOAT3& pos);
	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);

	void SetSRVIndex(UINT idx) { mSrvIndex = idx; }
	void SetLook(XMFLOAT3& look);
	void SetMesh(const std::shared_ptr<Mesh>& mesh) { mMeshes.push_back(mesh); }

	void SetRotation(XMFLOAT3& axis, float speed);
	void SetMovement(XMFLOAT3& dir, float speed);

	void SetReflected(XMFLOAT4& plane);
	void SetWorld(XMFLOAT4X4 world) { mWorld = world; }

public:
	virtual void PreDraw(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* rtvResource, GameScene* scene) { }
	
	virtual void BuildDsvRtvView(
		ID3D12Device* device,
		ID3D12Resource* rtvResource,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle) { }

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

	XMFLOAT4X4 GetWorld() const { return mWorld; }

	UINT GetSRVIndex() const { return mSrvIndex; }

	virtual ULONG GetCubeMapSize() const { return 0; }	
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
	std::vector<std::shared_ptr<Mesh>> mMeshes;

	BoundingOrientedBox mOOBB = { };

	GameObject* mParent = nullptr;
	GameObject* mChild = nullptr;
	GameObject* mSibling = nullptr;

	XMFLOAT3 mMoveDirection = {};
	XMFLOAT3 mRotationAxis = {};

	float mMoveSpeed = 0.0f;
	float mRotationSpeed = 0.0f;

	bool mReflected = false;
	XMFLOAT4X4 mReflectMatrix = Matrix4x4::Identity4x4();
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
	void BuildTerrainMesh(ID3D12Device* device,	ID3D12GraphicsCommandList* cmdList, int blockWidth, int blockDepth);

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
	Billboard(float width, float height);
	virtual ~Billboard();

	void AppendBillboard(const XMFLOAT3& pos);
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void SetDurationTime(std::chrono::milliseconds& time);
	bool IsTimeOver(std::chrono::steady_clock::time_point& currentTime);

private:
	float mWidth = 0.0f;
	float mHeight = 0.0f;

	std::vector<BillboardVertex> mVertices;
	std::vector<UINT> mIndices;
	
	std::chrono::steady_clock::time_point mCreationTime;
	std::chrono::milliseconds mDurationTime;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class DynamicCubeMapObject : public GameObject
{
public:
	DynamicCubeMapObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, LONG cubeMapSize);
	virtual ~DynamicCubeMapObject();

	virtual void BuildDsvRtvView(
		ID3D12Device* device,
		ID3D12Resource* rtvResource,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle) override;

	void BuildCameras();

	virtual void PreDraw(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* rtvResource, GameScene* scene) override;


public:
	virtual ULONG GetCubeMapSize() const { return mCubeMapSize; }

private:
	static const int RtvCounts = 6;

	ULONG mCubeMapSize = 0;

	std::array<std::unique_ptr<Camera>, RtvCounts> mCameras;

	D3D12_CPU_DESCRIPTOR_HANDLE mRtvCPUDescriptorHandles[RtvCounts];
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvCPUDescriptorHandle;

	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;
};