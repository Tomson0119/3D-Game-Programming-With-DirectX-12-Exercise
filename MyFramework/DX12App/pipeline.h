#pragma once

#include "Shader.h"
#include "gameObject.h"

class Pipeline
{
public:
	Pipeline();
	Pipeline(bool wired);
	Pipeline(const Pipeline& rhs) = delete;
	Pipeline& operator=(const Pipeline& rhs) = delete;
	virtual ~Pipeline();

	virtual void BuildPipeline(
		ID3D12Device* device, 
		ID3D12RootSignature* rootSig,
		Shader* shader);

	void SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) { mPrimitive = topology; }
	void SetObject(GameObject* obj);
	void SetAndDraw(ID3D12GraphicsCommandList* cmdList, ConstantBuffer<ObjectConstants>* objCB);

protected:
	ComPtr<ID3D12PipelineState> mPSO;

	D3D12_RASTERIZER_DESC	  mRasterizerDesc   = DefaultRasterizerDesc();
	D3D12_BLEND_DESC		  mBlendDesc		= DefaultBlendDesc();
	D3D12_DEPTH_STENCIL_DESC  mDepthStencilDesc = DefaultDepthStencilDesc();

	DXGI_FORMAT mBackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitive = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	std::vector<GameObject*> mRenderObjects;

	bool mIsWiredFrame = false;

private:
	static D3D12_RASTERIZER_DESC	DefaultRasterizerDesc();
	static D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc();
	static D3D12_BLEND_DESC			DefaultBlendDesc();
};