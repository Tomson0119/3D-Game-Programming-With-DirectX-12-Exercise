#include "stdafx.h"
#include "texture.h"


void Texture::CreateTextureResource(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const std::wstring& filePath,
	D3D12_RESOURCE_STATES resourceStates)
{
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool isCubeMap = false;

	ThrowIfFailed(LoadDDSTextureFromFileEx(
		device, filePath.c_str(), 0,
		D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT,
		mTexResource.GetAddressOf(), ddsData, subresources, &alphaMode, &isCubeMap));

	D3D12_HEAP_PROPERTIES heapProperties = Extension::HeapProperties(D3D12_HEAP_TYPE_UPLOAD);

	const UINT subresourcesCount = (UINT)subresources.size();
	UINT64 bytes = GetRequiredIntermediateSize(mTexResource.Get(), 0, subresourcesCount);

	D3D12_RESOURCE_DESC resourceDesc = Extension::BufferResourceDesc(bytes);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

	::UpdateSubresources(cmdList, mTexResource.Get(), mUploadBuffer.Get(),
		0, 0, subresourcesCount, subresources.data());

	D3D12_RESOURCE_BARRIER resourceBarrier = Extension::ResourceBarrier(
		mTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, resourceStates);
	cmdList->ResourceBarrier(1, &resourceBarrier);
}