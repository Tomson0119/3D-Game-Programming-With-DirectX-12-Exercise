#include "stdafx.h"
#include "pipeline.h"

Pipeline::Pipeline()
{
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
	psoDesc.SampleDesc.Count = 1;
	//psoDesc.SampleDesc.Quality = gMsaaStateDesc.Quality;

	if (mIsWiredFrame)
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void Pipeline::BuildConstantBuffer(ID3D12Device* device, UINT rootParameterIndex)
{
	mRootParamIndex = rootParameterIndex;
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, (UINT)mRenderObjects.size());
}

void Pipeline::BuildDescriptorHeap(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC decriptorHeapDesc = 
		Extension::DescriptorHeapDesc(mTextureCount + (UINT)mRenderObjects.size());

	ThrowIfFailed(device->CreateDescriptorHeap(
		&decriptorHeapDesc, IID_PPV_ARGS(&mCbvSrvDescriptorHeap)));

	BuildCBV(device);

}

void Pipeline::BuildCBV(ID3D12Device* device)
{
	UINT stride = sizeof(ObjectConstants);
}

void Pipeline::AppendObject(const std::shared_ptr<GameObject>& obj)
{
	mRenderObjects.push_back(obj);
}

void Pipeline::SetAndDraw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetPipelineState(mPSO.Get());

	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		auto address = mObjectCB->GetGPUVirtualAddress() +
			(UINT64)i * mObjectCB->GetByteSize();
		
		cmdList->SetGraphicsRootConstantBufferView(mRootParamIndex, address);
		mRenderObjects[i]->Draw(cmdList);
	}
}

void Pipeline::Update(const float elapsed)
{
	for (const auto& obj : mRenderObjects)
		obj->Update(elapsed, nullptr);
}

void Pipeline::UpdateConstants()
{
	for (int i = 0; i < mRenderObjects.size(); i++)
		mObjectCB->CopyData(i, mRenderObjects[i]->GetObjectConstants());
}