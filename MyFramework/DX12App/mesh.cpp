#include "stdafx.h"
#include "mesh.h"


Mesh::Mesh()
{

}

Mesh::Mesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::wstring& path)
{
	LoadFromBinary(device, cmdList, path);
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


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
BoxMesh::BoxMesh(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	float width, float height, float depth)
	: Mesh()
{
	float hx = width * 0.5f, hy = height * 0.5f, hz = depth * 0.5f;

	mOOBB.Center = { 0.0f,0.0f,0.0f };
	mOOBB.Extents = { hx,hy,hz };
	mOOBB.Orientation = { 0.0f,0.0f,0.0f,1.0f };

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
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
GridMesh::GridMesh(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList, 
	float width, float depth)
{
	float hw = 0.5f * width;
	float hd = 0.5f * depth;

	std::array<Vertex, 4> vertices = {
		Vertex(-hw, 0.0f, +hd, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(+hw, 0.0f, +hd, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f),
		Vertex(+hw, 0.0f, -hd, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f),
		Vertex(-hw, 0.0f, -hd, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f)
	};

	std::array<UINT, 6> indices = { 0, 1, 2, 0, 2, 3 };

	Mesh::CreateResourceInfo(device, cmdList, sizeof(Vertex), sizeof(UINT),
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
HeightMapGridMesh::HeightMapGridMesh(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	int xStart, int zStart,
	int width, int depth,
	const XMFLOAT3& scale,
	XMFLOAT4& color,
	HeightMapImage* context)
	: Mesh(), mWidth(width), mDepth(depth), mScale(scale)
{
	const UINT verticesCount = width * depth;
	const UINT indicesCount = (width * 2) * (depth - 1) + (depth - 1 - 1);

	std::vector<DiffuseVertex> vertices(verticesCount);
	std::vector<UINT> indices(indicesCount);

	size_t k = 0;
	for (int z = zStart; z < (zStart + depth); ++z)
	{
		for (int x = xStart; x < (xStart + width); ++x)
		{
			vertices[k].Position = XMFLOAT3((x * mScale.x), GetHeight(x, z, context), (z * mScale.z));
			vertices[k++].Color = Vector4::Add(color, GetColor(x, z, context));
		}
	}		

	k = 0;
	for (UINT z = 0; z < (UINT)depth - 1; ++z)
	{
		if (!(z & 1))
		{
			for (UINT x = 0; x < (UINT)width; ++x)
			{
				if ((x == 0) && (z > 0))
					indices[k++] = x + (z * width);
				indices[k++] = x + (z * width);
				indices[k++] = x + ((z + 1) * width);
			}
		}
		else
		{
			for (int x = width - 1; x >= 0; --x)
			{
				if (x == (width - 1))
					indices[k++] = x + (z * width);
				indices[k++] = x + (z * width);
				indices[k++] = x + ((z + 1) * width);
			}
		}
	}

	Mesh::CreateResourceInfo(device, cmdList, sizeof(DiffuseVertex), sizeof(UINT),
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
}

float HeightMapGridMesh::GetHeight(int x, int z, HeightMapImage* context) const
{
	float height = context->GetPixelValue(x + (z * mWidth));
	XMFLOAT3 scale = context->GetScale();
	return height * scale.y;
}

XMFLOAT4 HeightMapGridMesh::GetColor(int x, int z, HeightMapImage* context) const
{
	XMFLOAT3 litDir = Vector3::Normalize(XMFLOAT3(-1.0f, 1.0f, 1.0f));
	XMFLOAT3 scale = context->GetScale();
	XMFLOAT4 incidentLitColor = XMFLOAT4(0.9f, 0.8f, 0.4f, 1.0f);
	
	float reflect = Vector3::Dot(context->GetNormal(x, z), litDir);
	reflect += Vector3::Dot(context->GetNormal(x + 1, z), litDir);
	reflect += Vector3::Dot(context->GetNormal(x + 1, z + 1), litDir);
	reflect += Vector3::Dot(context->GetNormal(x, z + 1), litDir);

	reflect = (reflect / 4.0f) + 0.05f;
	reflect = Math::ClampFloat(reflect, 0.25f, 1.0f);
	
	return Vector4::Multiply(reflect, incidentLitColor);
}