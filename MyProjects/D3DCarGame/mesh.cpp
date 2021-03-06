#include "stdafx.h"
#include "mesh.h"

Mesh::Mesh()
{

}

Mesh::~Mesh()
{
}

void Mesh::CreateResourceInfo(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList,
	UINT vbStride, UINT ibStride,
	const void* vbData, UINT vbCount, 
	const void* ibData, UINT ibCount)
{
	const UINT vbByteSize = vbCount * vbStride;
	const UINT ibByteSize = ibCount * ibStride;

	mVertexBufferGPU = CreateBufferResource(device, cmdList,
		vbData, vbByteSize, mVertexUploadBuffer);

	mIndexBufferGPU = CreateBufferResource(device, cmdList,
		ibData, ibByteSize, mIndexUploadBuffer);

	mIndexCount = ibCount;
	mStartIndex = 0;
	mBaseVertex = 0;
	mSlot = 0;

	mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = vbByteSize;
	mVertexBufferView.StrideInBytes = vbStride;

	mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	mIndexBufferView.SizeInBytes = ibByteSize;
	mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void Mesh::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(mSlot, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
	cmdList->IASetPrimitiveTopology(mPrimitiveTopology);

	cmdList->DrawIndexedInstanced(mIndexCount, 1, mStartIndex, mBaseVertex, 0);
}

void Mesh::LoadFromText(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::wstring& path)
{
	std::ifstream file(path);

	assert(file.is_open() && L"Could not find file");

	std::vector<Vertex> vertices;
	std::vector<int> indices;

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texCoords;

	char ignore[64] = {};
	UINT verticesCount = 0;
	UINT indicesCount = 0;

	file >> ignore >> verticesCount;
	positions.resize(verticesCount);
	for (size_t i = 0; i < verticesCount; ++i)
		file >> positions[i].x >> positions[i].y >> positions[i].z;

	file >> ignore >> verticesCount;
	normals.resize(verticesCount);
	for (size_t i = 0; i < verticesCount; ++i)
		file >> normals[i].x >> normals[i].y >> normals[i].z;

	file >> ignore >> verticesCount;
	texCoords.resize(verticesCount);
	for (size_t i = 0; i < verticesCount; ++i)
		file >> texCoords[i].x >> texCoords[i].y;

	file >> ignore >> indicesCount;
	indices.resize(indicesCount);
	for (size_t i = 0; i < indicesCount; ++i)
		file >> indices[i];

	vertices.resize(verticesCount);
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Position = positions[i];
		vertices[i].Normal = normals[i];
		vertices[i].TexCoord = texCoords[i];
	}

	Mesh::CreateResourceInfo(device, cmdList, sizeof(Vertex), sizeof(UINT),
		vertices.data(), (UINT)vertices.size(),	indices.data(), (UINT)indices.size());
}

void Mesh::LoadFromBinary(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList,
	const std::wstring& path)
{
	std::ifstream file(path, std::ios::binary);

	assert(file.is_open() && L"Could not find file");

	char ignore[64] = {};
	UINT verticesCount = 0;
	UINT indicesCount = 0;

	std::vector<Vertex> vertices;
	std::vector<UINT> indices;

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texCoords;

	// =============== BoundingBox =============== //
	file.read(&ignore[0], sizeof(BYTE));
	file.read(&ignore[0], sizeof(char) * 14);  // '<BoundingBox>:'
	file.read(reinterpret_cast<char*>(&mOOBB.Center), sizeof(XMFLOAT3));
	file.read(reinterpret_cast<char*>(&mOOBB.Extents), sizeof(XMFLOAT3));

	// =============== Position =============== //
	file.read(&ignore[0], sizeof(BYTE));
	file.read(&ignore[0], sizeof(char) * 11);  // '<Vertices>:'
	file.read(reinterpret_cast<char*>(&verticesCount), sizeof(UINT));
	
	positions.resize(verticesCount);
	file.read(reinterpret_cast<char*>(&positions[0]), sizeof(XMFLOAT3) * verticesCount);

	// =============== Normal =============== //
	file.read(&ignore[0], sizeof(BYTE));
	file.read(&ignore[0], sizeof(char) * 10);  // '<Normals>:'
	file.read(reinterpret_cast<char*>(&verticesCount), sizeof(UINT));

	normals.resize(verticesCount);
	file.read(reinterpret_cast<char*>(&normals[0]), sizeof(XMFLOAT3) * verticesCount);

	// =============== TextureCoord =============== //
	file.read(&ignore[0], sizeof(BYTE));
	file.read(&ignore[0], sizeof(char) * 16);  // '<TextureCoords>:'
	file.read(reinterpret_cast<char*>(&verticesCount), sizeof(UINT));

	texCoords.resize(verticesCount);
	file.read(reinterpret_cast<char*>(&texCoords[0]), sizeof(XMFLOAT2) * verticesCount);

	// =============== Indices =============== //
	file.read(&ignore[0], sizeof(BYTE));
	file.read(&ignore[0], sizeof(char) * 10);  // '<Indices>:'
	file.read(reinterpret_cast<char*>(&indicesCount), sizeof(UINT));

	indices.resize(indicesCount);
	file.read(reinterpret_cast<char*>(&indices[0]), sizeof(UINT) * indicesCount);

	file.close();

	// =============== Vertices ============== //
	vertices.resize(verticesCount);
	for (size_t i = 0; i < verticesCount; ++i)
	{
		vertices[i].Position = positions[i];
		vertices[i].Normal = normals[i];
		vertices[i].TexCoord = texCoords[i];
	}

	Mesh::CreateResourceInfo(device, cmdList, sizeof(Vertex), sizeof(UINT),
		vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
BoxMesh::BoxMesh(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	float width, float height, float depth,
	bool colored)
	: Mesh()
{
	float hx = width * 0.5f, hy = height * 0.5f, hz = depth * 0.5f;

	mOOBB.Center = { 0.0f,0.0f,0.0f };
	mOOBB.Extents = { hx,hy,hz };
	mOOBB.Orientation = { 0.0f,0.0f,0.0f,1.0f };

	if (!colored) {
		std::array<Vertex, 24> vertices =
		{
			// Front
			Vertex(-hx, +hy, -hz, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),  // 0
			Vertex(+hx, +hy, -hz, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),  // 1
			Vertex(+hx, -hy, -hz, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),  // 2
			Vertex(-hx, -hy, -hz, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),  // 3

			// Back
			Vertex(+hx, +hy, +hz, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f),  // 4
			Vertex(-hx, +hy, +hz, 0.0f, 0.0f, +1.0f, 1.0f, 0.0f),  // 5
			Vertex(-hx, -hy, +hz, 0.0f, 0.0f, +1.0f, 1.0f, 1.0f),  // 6
			Vertex(+hx, -hy, +hz, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f),  // 7

			// Left
			Vertex(-hx, +hy, +hz, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f),  // 8
			Vertex(-hx, +hy, -hz, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f),  // 9
			Vertex(-hx, -hy, -hz, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f),  // 10
			Vertex(-hx, -hy, +hz, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f),  // 11

			// Right
			Vertex(+hx, +hy, -hz, +1.0f, 0.0f, 0.0f, 0.0f, 0.0f),  // 12
			Vertex(+hx, +hy, +hz, +1.0f, 0.0f, 0.0f, 1.0f, 0.0f),  // 13
			Vertex(+hx, -hy, +hz, +1.0f, 0.0f, 0.0f, 1.0f, 1.0f),  // 14
			Vertex(+hx, -hy, -hz, +1.0f, 0.0f, 0.0f, 0.0f, 1.0f),  // 15

			// Top
			Vertex(-hx, +hy, +hz, 0.0f, +1.0f, 0.0f, 0.0f, 0.0f),  // 16
			Vertex(+hx, +hy, +hz, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f),  // 17
			Vertex(+hx, +hy, -hz, 0.0f, +1.0f, 0.0f, 1.0f, 1.0f),  // 18
			Vertex(-hx, +hy, -hz, 0.0f, +1.0f, 0.0f, 0.0f, 1.0f),  // 19

			// Bottom
			Vertex(-hx, -hy, -hz, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f),  // 20
			Vertex(+hx, -hy, -hz, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f),  // 21
			Vertex(+hx, -hy, +hz, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f),  // 22
			Vertex(-hx, -hy, +hz, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f),  // 23
		};

		std::array<UINT, 36> indices =
		{
			// Front
			0, 1, 2, 0, 2, 3,
			// Back
			4, 5, 6, 4, 6, 7,
			// Left
			8, 9, 10, 8, 10, 11,
			// Right
			12, 13, 14, 12, 14, 15,
			// Top
			16, 17, 18, 16, 18, 19,
			// Bottom
			20, 21, 22, 20, 22, 23
		};

		Mesh::CreateResourceInfo(device, cmdList, sizeof(Vertex), sizeof(UINT),
			vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
	}
	else {
		std::array<ColoredVertex, 8> vertices =
		{
			ColoredVertex(XMFLOAT3(-hx, +hy, -hz), (XMFLOAT4)Colors::Red),
			ColoredVertex(XMFLOAT3(+hx, +hy, -hz), (XMFLOAT4)Colors::Blue),
			ColoredVertex(XMFLOAT3(+hx, -hy, -hz), (XMFLOAT4)Colors::Yellow),
			ColoredVertex(XMFLOAT3(-hx, -hy, -hz), (XMFLOAT4)Colors::Green),

			ColoredVertex(XMFLOAT3(+hx, +hy, +hz), (XMFLOAT4)Colors::Purple),
			ColoredVertex(XMFLOAT3(-hx, +hy, +hz), (XMFLOAT4)Colors::Black),
			ColoredVertex(XMFLOAT3(-hx, -hy, +hz), (XMFLOAT4)Colors::Gray),
			ColoredVertex(XMFLOAT3(+hx, -hy, +hz), (XMFLOAT4)Colors::Orange)
		};

		std::array<UINT, 36> indices =
		{
			// Front
			0, 1, 2, 0, 2, 3,
			// Back
			4, 5, 6, 4, 6, 7,
			// Top
			0, 5, 4, 0, 4, 1,
			// Bottom
			3, 7, 6, 3, 2, 7,
			// Left
			0, 6, 5, 0, 3, 6,
			// Right
			1, 4, 7, 1, 7, 2
		};

		Mesh::CreateResourceInfo(device, cmdList, sizeof(ColoredVertex), sizeof(UINT),
			vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
	}
}

BoxMesh::~BoxMesh()
{
}


///////////////////////////////////////////////////////////////////////////////////////////
//
CarMesh::CarMesh(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList,
	const std::wstring& path)
	: Mesh()
{
	LoadFromBinary(device, cmdList, path);
}
