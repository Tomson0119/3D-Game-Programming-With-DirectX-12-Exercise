#pragma once

#include "Shader.h"
#include "gameObject.h"

enum class Layer : int
{
	SkyBox,
	Terrain,
	NormalMapped,
	Default,
	Mirror,
	Reflected,
	Billboard,
	Transparent
	//DynamicCubeMap
};

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
		Shader* shader=nullptr);

	virtual void BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex);

	void BuildConstantBuffer(ID3D12Device* device);
	void BuildCBV(ID3D12Device* device);
	void BuildSRV(ID3D12Device* device);

	void SetWiredFrame(bool wired) { mIsWiredFrame = wired; }
	void SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) { mPrimitive = topology; }
	void SetCullModeBack();

	void SetAlphaBlending();
	void SetStencilOp(
		UINT stencilRef, D3D12_DEPTH_WRITE_MASK depthWriteMask,
		D3D12_STENCIL_OP stencilFail, D3D12_STENCIL_OP stencilDepthFail,
		D3D12_STENCIL_OP stencilPass, D3D12_COMPARISON_FUNC stencilFunc,
		UINT8 rtWriteMask);

	void AppendObject(const std::shared_ptr<GameObject>& obj);
	void AppendTexture(const std::shared_ptr<Texture>& tex);

	void DeleteObject(int idx);
	void ResetPipeline(ID3D12Device* device);

	virtual void Update(const float elapsed, Camera* camera=nullptr);
	virtual void SetAndDraw(ID3D12GraphicsCommandList* cmdList, bool drawWiredFrame = false);

	void UpdateConstants();

	const std::vector<std::shared_ptr<GameObject>>& GetRenderObjects() const { return mRenderObjects; }

protected:
	ComPtr<ID3D12PipelineState> mPSO[2];
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

	UINT mStencilRef = 0;
	bool mIsWiredFrame = false;
};


//////////////////////////////////////////////////////////////////////////////////
//
class SkyboxPipeline : public Pipeline
{
public:
	SkyboxPipeline(ID3D12Device *device, ID3D12GraphicsCommandList* cmdList);
	virtual ~SkyboxPipeline();

	virtual void BuildPipeline(
		ID3D12Device* device,
		ID3D12RootSignature* rootSig,
		Shader* shader = nullptr) override;
};