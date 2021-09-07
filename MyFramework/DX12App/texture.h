#pragma once

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() { }

	void CreateTextureResource(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::wstring& filePath,
		D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	void SetDimension(D3D12_SRV_DIMENSION dimension) { mViewDimension = dimension; }
	
public:
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView() const;
	ID3D12Resource* GetResource() const { return mTexResource.Get(); }
	
protected:
	ComPtr<ID3D12Resource> mTexResource;
	ComPtr<ID3D12Resource> mUploadBuffer;

	D3D12_SRV_DIMENSION mViewDimension{};

	DXGI_FORMAT mBufferFormats{};
	UINT mBufferElementsCount = 0;
};