#pragma once

#include "Shader.h"

class Pipeline
{
public:
	Pipeline();
	Pipeline(const Pipeline& rhs) = delete;
	Pipeline& operator=(const Pipeline& rhs) = delete;
	virtual ~Pipeline();

	void SetPipeline(ID3D12GraphicsCommandList* cmdList);
	void Draw(ID3D12GraphicsCommandList* cmdList);

	void BuildPipeline(
		ID3D12Device* device, 
		ID3D12RootSignature* rootSig,
		Shader* shader);

protected:
	ComPtr<ID3D12PipelineState> mPSO;

	D3D12_RASTERIZER_DESC	  mRasterizerDesc   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_BLEND_DESC		  mBlendDesc		= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC  mDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	DXGI_FORMAT mBackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitive = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
};