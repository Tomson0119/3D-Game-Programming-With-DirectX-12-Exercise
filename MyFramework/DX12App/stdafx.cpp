#include "stdafx.h"

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
