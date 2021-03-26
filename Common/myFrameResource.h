#pragma once

#include "myd3dUtil.h"
#include "myMathHelper.h"
#include "myUploadBuffer.h"

#include <memory>

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MyMathHelper::Identity4x4();
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MyMathHelper::Identity4x4();
	
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;

	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f,0.0f,0.0f,1.0f };

	DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 10.0f;
	float gFogRange = 200.0f;
	DirectX::XMFLOAT2 cbPerObjectPad2;

	Light Lights[MaxLights];
};

struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z, float nx, float ny, float nz,
		float u, float v) :
		Pos(x, y, z),
		Normal(nx, ny, nz),
		TexC(u, v) { }

	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 TexC;
};

struct MyFrameResource
{
public:

	MyFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	MyFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT waveVertCount);
	MyFrameResource(const MyFrameResource& rhs) = delete;
	MyFrameResource& operator=(const MyFrameResource& rhs) = delete;
	~MyFrameResource();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// We cannot update a cbuffer until the GPU is done processing the commands
	// that reference it. So each frame needs their own cbuffers.
	std::unique_ptr<MyUploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr < MyUploadBuffer<MaterialConstants>> MaterialCB = nullptr;
	std::unique_ptr<MyUploadBuffer<PassConstants>> PassCB = nullptr;

	// We cannot update a dynamic vertex buffer until the GPU is done processing
	// the commands that reference it. So each frame needs their own.
	std::unique_ptr<MyUploadBuffer<Vertex>> WavesVB = nullptr;

	// Fence value to mark commands up to this fence point. This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
};