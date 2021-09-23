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

void Pipeline::BuildConstantBuffer(ID3D12Device* device)
{
	std::cout << "BuildObjectCB\n";
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, (UINT)mRenderObjects.size());
}

void Pipeline::BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex)
{
	D3D12_DESCRIPTOR_HEAP_DESC decriptorHeapDesc = 
		Extension::DescriptorHeapDesc((UINT)mTextures.size() + (UINT)mRenderObjects.size());

	ThrowIfFailed(device->CreateDescriptorHeap(
		&decriptorHeapDesc, IID_PPV_ARGS(&mCbvSrvDescriptorHeap)));

	mCbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mRootParamCBVIndex = cbvIndex;
	mRootParamSRVIndex = srvIndex;

	BuildCBV(device);
	BuildSRV(device);
}

void Pipeline::BuildCBV(ID3D12Device* device)
{
	UINT stride = mObjectCB->GetByteSize();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.SizeInBytes = stride;
	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		cbvDesc.BufferLocation = mObjectCB->GetGPUVirtualAddress() + (stride * i);
		
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += i * mCbvSrvDescriptorSize;
		device->CreateConstantBufferView(&cbvDesc, cpuHandle);
	}
}

void Pipeline::BuildSRV(ID3D12Device* device)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += mRenderObjects.size() * mCbvSrvDescriptorSize;

	for (int i = 0; i < mTextures.size(); i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = mTextures[i]->ShaderResourceView();
		device->CreateShaderResourceView(mTextures[i]->GetResource(), &srvDesc, cpuHandle);
		cpuHandle.ptr += mCbvSrvDescriptorSize;
	}
}

void Pipeline::AppendObject(const std::shared_ptr<GameObject>& obj)
{
	mRenderObjects.push_back(obj);
}

void Pipeline::AppendTexture(const std::shared_ptr<Texture>& tex)
{
	mTextures.push_back(tex);
}

void Pipeline::SetAndDraw(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* descHeaps[] = { mCbvSrvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	cmdList->SetPipelineState(mPSO.Get());

	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		auto gpuHandle = mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += i * mCbvSrvDescriptorSize;
		cmdList->SetGraphicsRootDescriptorTable(mRootParamCBVIndex, gpuHandle);

		if (mTextures.size() > 0)
		{
			gpuHandle = mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			gpuHandle.ptr += (mRenderObjects.size() + mRenderObjects[i]->GetSRVIndex()) * mCbvSrvDescriptorSize;
			cmdList->SetGraphicsRootDescriptorTable(mRootParamSRVIndex, gpuHandle);
		}		

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