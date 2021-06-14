#pragma once

#include "heightMapImage.h"

struct Vertex
{
	Vertex() = default;
	Vertex(float x,  float y,  float z,
		   float nx, float ny, float nz,
		   float u,  float v)
		: Position(x, y, z), Normal(nx, ny, nz), TexCoord(u, v) { }

	Vertex(XMFLOAT3 pos, XMFLOAT3 normal, XMFLOAT2 texC)
		: Position(pos), Normal(normal), TexCoord(texC) { }

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
};

class Mesh 
{
public:
	Mesh();
	virtual ~Mesh();

	void CreateResourceInfo(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT vbStride, UINT ibStride,
		const void* vbData, UINT vbCount,
		const void* ibData, UINT ibCount);

	void Draw(ID3D12GraphicsCommandList* cmdList);

	void LoadFromText(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::wstring& path);

	void LoadFromBinary(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::wstring& path);
	
protected:
	ComPtr<ID3D12Resource> mVertexBufferGPU;
	ComPtr<ID3D12Resource> mIndexBufferGPU;

	ComPtr<ID3D12Resource> mVertexUploadBuffer;
	ComPtr<ID3D12Resource> mIndexUploadBuffer;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	UINT mSlot = 0;
	UINT mIndexCount = 0;
	UINT mStartIndex = 0;
	UINT mBaseVertex = 0;

public:
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	BoundingOrientedBox mOOBB;
};

class BoxMesh : public Mesh
{
public:
	BoxMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		float width, float height, float depth);
	virtual ~BoxMesh();
};

class GridMesh : public Mesh
{
public:
	GridMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		int width, int depth,
		const XMFLOAT3& scale,
		const std::wstring& heightMapFile=L"");

	virtual ~GridMesh();

private:
	std::unique_ptr<HeightMapImage> mHeightMap;

	int mWidth = 0;
	int mDepth = 0;
	XMFLOAT3 mScale = {};
};
