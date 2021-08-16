#include "stdafx.h"
#include "mesh.h"


Mesh::Mesh()
{

}

Mesh::Mesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::wstring& path)
{
	LoadFromBinary(device, cmdList, path);
}

Mesh::~Mesh()
{
}

void Mesh::CreateResourceInfo(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList,
	UINT vbStride, UINT ibStride,
	D3D12_PRIMITIVE_TOPOLOGY topology,
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

	mPrimitiveTopology = topology;

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
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
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
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
}