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

protected:
	ComPtr<ID3D12Resource> mTexResource;
	ComPtr<ID3D12Resource> mUploadBuffer;
};