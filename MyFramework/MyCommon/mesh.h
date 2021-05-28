#pragma once

struct Vertex
{
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

struct ColoredVertex
{
	ColoredVertex() = default;
	ColoredVertex(XMFLOAT3 pos, XMFLOAT4 color)
		: Position(pos), Color(color) { }

	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

class Mesh // MeshGeometry
{
public:
	Mesh();
	virtual ~Mesh();

	void CreateResourceInfo(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT vbStride,
		const void* vbData, UINT vbCount,
		const void* ibData, UINT ibCount);
	
protected:
	ComPtr<ID3D12Resource> mVertexBufferGPU;
	ComPtr<ID3D12Resource> mIndexBufferGPU;

	ComPtr<ID3D12Resource> mVertexUploadBuffer;
	ComPtr<ID3D12Resource> mIndexUploadBuffer;

	DXGI_FORMAT mIndexFormat = {};
	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	UINT mVertexByteStride = 0;
	UINT mSlot = 0;

	UINT mIndexCount = 0;
	UINT mStartIndex = 0;
	UINT mBaseVertex = 0;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	BoundingOrientedBox mOOBB;
};

class BoxMesh : public Mesh
{
public:
	BoxMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		float width, float height, float depth);
	BoxMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList, 
		float width, float height, float depth, XMFLOAT4 color);
	virtual ~BoxMesh();
};