#include "myFrameResource.h"

MyFrameResource::MyFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = std::make_unique<MyUploadBuffer<PassConstants>>(device, passCount, true);
	ObjectCB = std::make_unique<MyUploadBuffer<ObjectConstants>>(device, objectCount, true);
	MaterialCB = std::make_unique<MyUploadBuffer<MaterialConstants>>(device, materialCount, true);
}

MyFrameResource::MyFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT waveVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = std::make_unique<MyUploadBuffer<PassConstants>>(device, passCount, true);
	ObjectCB = std::make_unique<MyUploadBuffer<ObjectConstants>>(device, objectCount, true);
	MaterialCB = std::make_unique<MyUploadBuffer<MaterialConstants>>(device, materialCount, true);

	WavesVB = std::make_unique<MyUploadBuffer<Vertex>>(device, waveVertCount, false);
}

MyFrameResource::~MyFrameResource()
{
}
