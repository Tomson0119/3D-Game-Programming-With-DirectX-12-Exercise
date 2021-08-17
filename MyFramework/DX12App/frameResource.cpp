#include "stdafx.h"
#include "frameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT objectCount, UINT cameraCount, UINT lightCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&mCmdAlloc)));

	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, objectCount);
	
}

FrameResource::~FrameResource()
{
}
