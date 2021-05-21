#include "../MyCommon/stdafx.h"
#include "pipeline.h"

extern DXGI_SAMPLE_DESC gMsaaStateDesc;

Pipeline::Pipeline()
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::SetPipeline(ID3D12GraphicsCommandList* cmdList)
{
	mCommandList->
}

void Pipeline::Draw(ID3D12GraphicsCommandList* cmdList)
{
}

void Pipeline::BuildPipeline(
	ID3D12Device* device,
	ID3D12RootSignature* rootSig,
	Shader* shader)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	auto layout = &shader->GetInputLayout();

	psoDesc.pRootSignature = rootSig;
	psoDesc.InputLayout = { 
		layout->data(),
		(UINT)layout->size()
	};
	psoDesc.VS = {
		shader->GetVS()->GetBufferPointer(),
		shader->GetVS()->GetBufferSize()
	};
	psoDesc.PS = {
		shader->GetPS()->GetBufferPointer(),
		shader->GetPS()->GetBufferSize()
	};
	psoDesc.RasterizerState = mRasterizerDesc;
	psoDesc.BlendState = mBlendDesc;
	psoDesc.DepthStencilState = mDepthStencilDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = mPrimitive;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.SampleDesc.Count = gMsaaStateDesc.Count;
	psoDesc.SampleDesc.Quality = gMsaaStateDesc.Quality;

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO)));
}
