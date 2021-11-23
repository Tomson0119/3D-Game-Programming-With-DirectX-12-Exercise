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
	if (shader->GetGS() != nullptr)
	{
		psoDesc.GS = {
			reinterpret_cast<BYTE*>(shader->GetGS()->GetBufferPointer()),
			shader->GetGS()->GetBufferSize()
		};
	}
	if (shader->GetDS() != nullptr)
	{
		psoDesc.DS = {
			reinterpret_cast<BYTE*>(shader->GetDS()->GetBufferPointer()),
			shader->GetDS()->GetBufferSize()
		};
	}
	if (shader->GetHS() != nullptr)
	{
		psoDesc.HS = {
			reinterpret_cast<BYTE*>(shader->GetHS()->GetBufferPointer()),
			shader->GetHS()->GetBufferSize()
		};
	}
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

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO[0])));

	if (mIsWiredFrame) {
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ThrowIfFailed(device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&mPSO[1])));
	}
}

void Pipeline::BuildConstantBuffer(ID3D12Device* device)
{
	if(mRenderObjects.size() > 0)
		mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, (UINT)mRenderObjects.size());
}

void Pipeline::BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex)
{
	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(
			(UINT)mTextures.size() + (UINT)mRenderObjects.size(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
		IID_PPV_ARGS(&mCbvSrvDescriptorHeap)));

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
		cbvDesc.BufferLocation = mObjectCB->GetGPUVirtualAddress(0) + (stride * i);
		
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += i * gCbvSrvUavDescriptorSize;
		device->CreateConstantBufferView(&cbvDesc, cpuHandle);
	}
}

void Pipeline::BuildSRV(ID3D12Device* device)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += mRenderObjects.size() * gCbvSrvUavDescriptorSize;

	for (int i = 0; i < mTextures.size(); i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = mTextures[i]->ShaderResourceView();
		device->CreateShaderResourceView(mTextures[i]->GetResource(), &srvDesc, cpuHandle);
		cpuHandle.ptr += gCbvSrvUavDescriptorSize;
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

void Pipeline::DeleteObject(int idx)
{
	mRenderObjects.erase(mRenderObjects.begin() + idx);
}

void Pipeline::ResetPipeline(ID3D12Device* device)
{
	BuildConstantBuffer(device);
	BuildDescriptorHeap(device, mRootParamCBVIndex, mRootParamSRVIndex);
}

void Pipeline::SetAndDraw(ID3D12GraphicsCommandList* cmdList, bool drawWiredFrame)
{
	ID3D12DescriptorHeap* descHeaps[] = { mCbvSrvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	cmdList->OMSetStencilRef(mStencilRef);

	if(mIsWiredFrame && drawWiredFrame)
		cmdList->SetPipelineState(mPSO[1].Get());
	else
		cmdList->SetPipelineState(mPSO[0].Get());

	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		auto gpuHandle = mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += i * gCbvSrvUavDescriptorSize;
		cmdList->SetGraphicsRootDescriptorTable(mRootParamCBVIndex, gpuHandle);

		if (mTextures.size() > 0)
		{
			gpuHandle = mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			gpuHandle.ptr += (mRenderObjects.size() + mRenderObjects[i]->GetSRVIndex()) * gCbvSrvUavDescriptorSize;
			cmdList->SetGraphicsRootDescriptorTable(mRootParamSRVIndex, gpuHandle);
		}		

		mRenderObjects[i]->Draw(cmdList);
	}
}

void Pipeline::SetCullModeBack()
{
	mRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	mRasterizerDesc.FrontCounterClockwise = true;
}

void Pipeline::SetAlphaBlending()
{
	D3D12_RENDER_TARGET_BLEND_DESC rtBlend{};
	rtBlend.BlendEnable = TRUE;
	rtBlend.LogicOpEnable = FALSE;
	rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDesc.RenderTarget[0] = rtBlend;
}

void Pipeline::SetStencilOp(
	UINT stencilRef, D3D12_DEPTH_WRITE_MASK depthWriteMask,
	D3D12_STENCIL_OP stencilFail, D3D12_STENCIL_OP stencilDepthFail, 
	D3D12_STENCIL_OP stencilPass, D3D12_COMPARISON_FUNC stencilFunc, UINT8 rtWriteMask)
{
	D3D12_DEPTH_STENCILOP_DESC depthStencilDesc{};
	depthStencilDesc.StencilFailOp = stencilFail;
	depthStencilDesc.StencilDepthFailOp = stencilDepthFail;
	depthStencilDesc.StencilPassOp = stencilPass;
	depthStencilDesc.StencilFunc = stencilFunc;
	
	mDepthStencilDesc.StencilEnable = TRUE;
	mDepthStencilDesc.DepthWriteMask = depthWriteMask;
	mDepthStencilDesc.FrontFace = depthStencilDesc;
	mDepthStencilDesc.BackFace = depthStencilDesc;
	mBlendDesc.RenderTarget[0].RenderTargetWriteMask = rtWriteMask;

	mStencilRef = stencilRef;
}

void Pipeline::Update(const float elapsed, Camera* camera)
{
	for (const auto& obj : mRenderObjects)
		obj->Update(elapsed, nullptr);
}

void Pipeline::UpdateConstants()
{
	for (int i = 0; i < mRenderObjects.size(); i++)
		mObjectCB->CopyData(i, mRenderObjects[i]->GetObjectConstants());
}


//////////////////////////////////////////////////////////////////////////////////////////
//
SkyboxPipeline::SkyboxPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
	: Pipeline()
{
	auto skyboxTex = std::make_shared<Texture>();
	skyboxTex->LoadTextureFromDDS(device, cmdList, L"Resources\\skyboxarray.dds");
	skyboxTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mTextures.push_back(skyboxTex);

	auto boxMesh = std::make_shared<BoxMesh>(device, cmdList, 20.0f, 20.0f, 20.0f);
	auto skyboxObj = std::make_shared<GameObject>();
	skyboxObj->SetMesh(boxMesh);
	skyboxObj->SetSRVIndex(0);
	mRenderObjects.push_back(skyboxObj);
}

SkyboxPipeline::~SkyboxPipeline()
{
}

void SkyboxPipeline::BuildPipeline(ID3D12Device* device, ID3D12RootSignature* rootSig, Shader* shader)
{
	auto skyboxShader = std::make_shared<DefaultShader>(L"Shaders\\skybox.hlsl");
		
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	auto layout = skyboxShader->GetInputLayout();
	psoDesc.pRootSignature = rootSig;
	psoDesc.InputLayout = {
		layout.data(),
		(UINT)layout.size()
	};
	psoDesc.VS = {
		reinterpret_cast<BYTE*>(skyboxShader->GetVS()->GetBufferPointer()),
		skyboxShader->GetVS()->GetBufferSize()
	};
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(skyboxShader->GetPS()->GetBufferPointer()),
		skyboxShader->GetPS()->GetBufferSize()
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

	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO[0])));
}