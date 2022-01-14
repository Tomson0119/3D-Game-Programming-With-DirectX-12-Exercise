#include "stdafx.h"
#include "texture.h"


void Texture::LoadTextureFromDDS(
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

	const UINT subresourcesCount = (UINT)subresources.size();
	UINT64 bytes = GetRequiredIntermediateSize(mTexResource.Get(), 0, subresourcesCount);

	ThrowIfFailed(device->CreateCommittedResource(
		&Extension::HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&Extension::BufferResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, bytes),
		D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

	::UpdateSubresources(cmdList, mTexResource.Get(), mUploadBuffer.Get(),
		0, 0, subresourcesCount, subresources.data());

	D3D12_RESOURCE_BARRIER resourceBarrier = Extension::ResourceBarrier(
		mTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, resourceStates);
	cmdList->ResourceBarrier(1, &resourceBarrier);
}

void Texture::CreateTexture(
	ID3D12Device* device, 
	UINT width, UINT height, UINT elements, UINT miplevels,
	DXGI_FORMAT format, D3D12_RESOURCE_FLAGS resourceFlags, 
	D3D12_RESOURCE_STATES resourceStates, D3D12_CLEAR_VALUE* clearValue)
{
	mTexResource = CreateTexture2DResource(device, width, height, elements, miplevels, format, resourceFlags, resourceStates, clearValue);
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture::ShaderResourceView() const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mTexResource->GetDesc().Format;
	srvDesc.ViewDimension = mViewDimension;

	switch (mViewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE2D:
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
		srvDesc.Texture2DArray.MipLevels = -1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = mTexResource->GetDesc().DepthOrArraySize;
		break;

	case D3D12_SRV_DIMENSION_TEXTURECUBE:
		srvDesc.TextureCube.MipLevels = -1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;

	case D3D12_SRV_DIMENSION_BUFFER:
		srvDesc.Format = mBufferFormats;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = mBufferElementsCount;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return srvDesc;
}
