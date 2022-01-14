#pragma once

#include "pipeline.h"

class GameScene;

class ShadowMapRenderer : public Pipeline
{
public:
	ShadowMapRenderer(ID3D12Device* device, UINT width, UINT height, UINT lightCount);
	virtual ~ShadowMapRenderer();

	virtual void BuildPipeline(ID3D12Device* device, ID3D12RootSignature* rootSig, Shader* shader=nullptr) override;
	virtual void BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex) override;
	
	void UpdateDepthCamera(LightConstants& lightCnst);
	void PreRender(ID3D12GraphicsCommandList* cmdList, GameScene* scene);
	void RenderPipelines(ID3D12GraphicsCommandList* cmdList);

	void AppendTargetPipeline(Pipeline* pso) { mShadowTargetPSOs.push_back(pso); }
	void SetShadowMapSRV(ID3D12GraphicsCommandList* cmdList, UINT srvIndex);

	void SetSunRange(float range) { mSunRange = range; }
	void SetCenter(const XMFLOAT3& center) { mCenter = center; }

	XMFLOAT4X4 GetShadowTransform(int idx) const;

private:
	void CreateTexture(ID3D12Device* device);
	void BuildDescriptorViews(ID3D12Device* device);

private:
	UINT mMapWidth;
	UINT mMapHeight;

	XMFLOAT3 mCenter;
	float mSunRange;

	UINT mMapCount;

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;

	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mRtvCPUDescriptorHandles;
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvCPUDescriptorHandle;

	ComPtr<ID3D12Resource> mDepthBuffer;

	std::vector<std::unique_ptr<Camera>> mDepthCamera;
	std::vector<Pipeline*> mShadowTargetPSOs;

	const XMFLOAT4X4 mToTexture =
	{
		0.5f,  0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 1.0f, 0.0f,
		0.5f,  0.5f, 0.0f, 1.0f
	};

	const int OrthographicPlaneWidth = 1024;
	const int OrthographicPlaneHeight = 1024;
};