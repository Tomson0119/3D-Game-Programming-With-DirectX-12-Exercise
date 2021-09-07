#pragma once

#include "Shader.h"
#include "gameObject.h"

class Pipeline
{
public:
	Pipeline();
	Pipeline(const Pipeline& rhs) = delete;
	Pipeline& operator=(const Pipeline& rhs) = delete;
	virtual ~Pipeline();

	virtual void BuildPipeline(
		ID3D12Device* device, 
		ID3D12RootSignature* rootSig,
		Shader* shader);

	void BuildConstantBuffer(ID3D12Device* device);
	void BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex);
	void BuildCBV(ID3D12Device* device);
	void BuildSRV(ID3D12Device* device);

	void SetWiredFrame(bool wired) { mIsWiredFrame = wired; }
	void SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) { mPrimitive = topology; }
	void SetAndDraw(ID3D12GraphicsCommandList* cmdList);

	void AppendObject(const std::shared_ptr<GameObject>& obj);
	void AppendTexture(const std::shared_ptr<Texture>& tex);

	void Update(const float elapsed);
	void UpdateConstants();

protected:
	ComPtr<ID3D12PipelineState> mPSO;
	ComPtr<ID3D12DescriptorHeap> mCbvSrvDescriptorHeap;

	D3D12_RASTERIZER_DESC	  mRasterizerDesc   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_BLEND_DESC		  mBlendDesc		= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC  mDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	DXGI_FORMAT mBackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitive = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	std::unique_ptr<ConstantBuffer<ObjectConstants>> mObjectCB;
	std::vector<std::shared_ptr<GameObject>> mRenderObjects;
	std::vector<std::shared_ptr<Texture>> mTextures;
	
	UINT mRootParamCBVIndex = 0;
	UINT mRootParamSRVIndex = 0;
	UINT mCbvSrvDescriptorSize = 0;

	bool mIsWiredFrame = false;
};