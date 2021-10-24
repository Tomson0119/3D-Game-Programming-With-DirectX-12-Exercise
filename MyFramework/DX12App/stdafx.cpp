#include "stdafx.h"


UINT gRtvDescriptorSize = 0;
UINT gDsvDescriptorSize = 0;
UINT gCbvSrvUavDescriptorSize = 0;

ComPtr<ID3D12Resource> CreateBufferResource(
    ID3D12Device* device, 
    ID3D12GraphicsCommandList* cmdList,
    const void* initData, UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultResource;

    ThrowIfFailed(device->CreateCommittedResource(
        &Extension::HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &Extension::BufferResourceDesc(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST, 
		nullptr, IID_PPV_ARGS(defaultResource.GetAddressOf())));

	ThrowIfFailed(device->CreateCommittedResource(
		&Extension::HeapProperties(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&Extension::BufferResourceDesc(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// 기본 버퍼에 넣을 데이터를 서술하는 구조체
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = initData;
	subresourceData.RowPitch = byteSize;
	subresourceData.SlicePitch = byteSize;

	UpdateSubresources(cmdList, defaultResource.Get(), 
		uploadBuffer.Get(), 0, 0, 1, &subresourceData);

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		defaultResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	return defaultResource;
}

ComPtr<ID3D12Resource> CreateTexture2DResource(
	ID3D12Device* device,
	UINT width, UINT height, UINT elements, UINT miplevels, 
	DXGI_FORMAT format, D3D12_RESOURCE_FLAGS resourceFlags, 
	D3D12_RESOURCE_STATES resourceStates, D3D12_CLEAR_VALUE* clearValue)
{
	ComPtr<ID3D12Resource> textureResource;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = elements;
	resourceDesc.MipLevels = miplevels;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = resourceFlags;

	ThrowIfFailed(device->CreateCommittedResource(
		&Extension::HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		resourceStates, clearValue, IID_PPV_ARGS(&textureResource)));

	return textureResource;
}
