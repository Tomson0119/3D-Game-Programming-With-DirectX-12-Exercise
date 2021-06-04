#include "../MyCommon/stdafx.h"
#include "pipeline.h"

extern DXGI_SAMPLE_DESC gMsaaStateDesc;

Pipeline::Pipeline()
{
}

Pipeline::Pipeline(bool wired)
{
	mIsWiredFrame = wired;
}

Pipeline::~Pipeline()
{
}

void Pipeline::BuildPipeline(
	ID3D12Device* device,
	ID3D12RootSignature* rootSig,
	Shader* shader)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	auto layout = shader->GetInputLayout();

	psoDesc.pRootSignature = rootSig;
	psoDesc.InputLayout = {
		layout.data(),
		(UINT)layout.size()
	};
	psoDesc.VS = {
		reinterpret_cast<BYTE*>(shader->GetVS()->GetBufferPointer()),
		shader->GetVS()->GetBufferSize()
	};
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(shader->GetPS()->GetBufferPointer()),
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

	if (mIsWiredFrame)
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void Pipeline::SetObject(GameObject* obj)
{
	mRenderObjects.push_back(obj);
}

void Pipeline::SetAndDraw(ID3D12GraphicsCommandList* cmdList, ConstantBuffer<ObjectConstants>* objCB)
{
	cmdList->SetPipelineState(mPSO.Get());

	for (const auto& item : mRenderObjects) {
		auto address = objCB->GetGPUVirtualAddress() 
			+ (UINT64)item->CBIndex() * objCB->GetByteSize();
		cmdList->SetGraphicsRootConstantBufferView(2, address);
		item->Draw(cmdList);
	}
}