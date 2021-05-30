#include "../MyCommon/stdafx.h"
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
	UINT vbStride,
	const void* vbData, UINT vbCount, 
	const void* ibData, UINT ibCount)
{
	const UINT vbByteSize = vbCount * vbStride;
	const UINT ibByteSize = ibCount * sizeof(std::uint16_t);

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
	mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void Mesh::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(mSlot, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
	cmdList->IASetPrimitiveTopology(mPrimitiveTopology);

	cmdList->DrawIndexedInstanced(mIndexCount, 1, mStartIndex, mBaseVertex, 0);
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

		std::array<std::uint16_t, 36> indices =
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

		Mesh::CreateResourceInfo(device, cmdList, sizeof(Vertex),
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

		std::array<std::uint16_t, 36> indices =
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

		Mesh::CreateResourceInfo(device, cmdList, sizeof(ColoredVertex),
			vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
	}
}

BoxMesh::~BoxMesh()
{
}