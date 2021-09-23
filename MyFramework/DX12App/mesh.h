#pragma once

#include "heightMapImage.h"

struct Vertex
{
	Vertex() = default;
	Vertex(float x,  float y,  float z,
		   float nx, float ny, float nz,
		   float u,  float v)
		: Position(x, y, z), Normal(nx, ny, nz), TexCoord(u, v) { }

	Vertex(const XMFLOAT3& pos,
		   const XMFLOAT3& normal,
		   const XMFLOAT2& texC)
		: Position(pos), Normal(normal), TexCoord(texC) { }

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
};

struct DiffuseVertex
{
	DiffuseVertex() = default;
	DiffuseVertex(float x, float y, float z,
				  const XMFLOAT4& color)
		: Position(x, y, z), Color(color) { }
	DiffuseVertex(const XMFLOAT3& pos,
				  const XMFLOAT4& color)
		: Position(pos), Color(color) { }

	XMFLOAT3 Position;
	XMFLOAT4 Color;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class Mesh 
{
public:
	Mesh();
	Mesh(ID3D12Device* device,
		 ID3D12GraphicsCommandList* cmdList,
		 const std::wstring& path);
	virtual ~Mesh() { }

	void CreateResourceInfo(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT vbStride, UINT ibStride,
		D3D12_PRIMITIVE_TOPOLOGY topology,
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class BoxMesh : public Mesh
{
public:
	BoxMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		float width, float height, float depth);
	virtual ~BoxMesh() { }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class GridMesh : public Mesh
{
public:
	GridMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		float width, float depth);
	virtual ~GridMesh() { }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
class HeightMapGridMesh : public Mesh
{
public:
	HeightMapGridMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		int xStart, int zStart,
		int width, int depth,
		const XMFLOAT3& scale,
		XMFLOAT4& color,
		HeightMapImage* context);

	virtual ~HeightMapGridMesh() { }

	float GetHeight(int x, int z, HeightMapImage* context) const;
	XMFLOAT4 GetColor(int x, int z, HeightMapImage* context) const;
	
private:
	XMFLOAT3 mScale = {};

	int mWidth = 0;
	int mDepth = 0;	
};
