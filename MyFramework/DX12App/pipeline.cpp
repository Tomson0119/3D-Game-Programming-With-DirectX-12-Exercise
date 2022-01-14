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
	if (shader->GetPS() != nullptr) 
	{
		psoDesc.PS = {
			reinterpret_cast<BYTE*>(shader->GetPS()->GetBufferPointer()),
			shader->GetPS()->GetBufferSize()
		};
	}
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
	if (mObjectCB == nullptr) return;

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

void Pipeline::SetAndDraw(ID3D12GraphicsCommandList* cmdList, bool drawWiredFrame, bool setPipeline)
{
	ID3D12DescriptorHeap* descHeaps[] = { mCbvSrvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	cmdList->OMSetStencilRef(mStencilRef);
	
	if (setPipeline) {
		if (mIsWiredFrame && drawWiredFrame)
			cmdList->SetPipelineState(mPSO[1].Get());
		else
			cmdList->SetPipelineState(mPSO[0].Get());
	}

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

void Pipeline::SetCullClockwise()
{
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


/////////////////////////////////////////////////////////////////////////
//
StreamOutputPipeline::StreamOutputPipeline()
	: Pipeline(), mStreamOutputDesc({})
{
}

StreamOutputPipeline::~StreamOutputPipeline()
{
}

void StreamOutputPipeline::BuildPipeline(ID3D12Device* device, ID3D12RootSignature* rootSig, Shader* shader)
{
	auto renderShader = std::make_unique<BillboardShader>(L"Shaders\\billboard.hlsl");
	auto soShader = std::make_unique<BillboardShader>(L"Shaders\\billboard.hlsl", true);

	SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	SetAlphaBlending();
	CreateStreamOutputDesc();

	Pipeline::BuildPipeline(device, rootSig, renderShader.get());
	BuildSOPipeline(device, rootSig, soShader.get());
}

void StreamOutputPipeline::SetAndDraw(
	ID3D12GraphicsCommandList* cmdList, 
	bool drawWiredFrame, 
	bool setPipeline)
{
	ID3D12DescriptorHeap* descHeaps[] = { mCbvSrvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	cmdList->OMSetStencilRef(mStencilRef);
	
	cmdList->SetPipelineState(mPSO[1].Get());
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

		mRenderObjects[i]->ExecuteSO(cmdList);
	}

	cmdList->SetPipelineState(mPSO[0].Get()); // Draw
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

void StreamOutputPipeline::BuildSOPipeline(ID3D12Device* device, ID3D12RootSignature* rootSig, Shader* shader)
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
	psoDesc.GS = {
		reinterpret_cast<BYTE*>(shader->GetGS()->GetBufferPointer()),
		shader->GetGS()->GetBufferSize()
	};
	psoDesc.PS = { NULL, 0 };
	psoDesc.RasterizerState = mRasterizerDesc;
	psoDesc.BlendState = mBlendDesc;
	psoDesc.DepthStencilState = mDepthStencilDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = mPrimitive;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.StreamOutput = mStreamOutputDesc;
	//psoDesc.SampleDesc.Quality = gMsaaStateDesc.Quality;

	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSO[1])));
}

void StreamOutputPipeline::CreateStreamOutputDesc()
{
	mSODeclarations.push_back({ 0, "POSITION",  0, 0, 3, 0 });
	mSODeclarations.push_back({ 0, "SIZE",      0, 0, 2, 0 });
	mSODeclarations.push_back({ 0, "DIRECTION", 0, 0, 3, 0 });
	mSODeclarations.push_back({ 0, "LIFETIME",  0, 0, 2, 0 });
	mSODeclarations.push_back({ 0, "SPEED",	    0, 0, 1, 0 });
	mSODeclarations.push_back({ 0, "TYPE",	    0, 0, 1, 0 });
	
	mStrides.push_back(sizeof(BillboardVertex));

	ZeroMemory(&mStreamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));
	mStreamOutputDesc.NumEntries = (UINT)mSODeclarations.size();
	mStreamOutputDesc.NumStrides = (UINT)mStrides.size();
	mStreamOutputDesc.pBufferStrides = mStrides.data();
	mStreamOutputDesc.pSODeclaration = mSODeclarations.data();
	mStreamOutputDesc.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
}


/////////////////////////////////////////////////////////////////////////
//
ComputePipeline::ComputePipeline(ID3D12Device* device)
{
	CreateTextures(device);
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::BuildPipeline(
	ID3D12Device* device, 
	ID3D12RootSignature* rootSig, 
	ComputeShader* shader)
{
	mComputeRootSig = rootSig;

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSig;
	psoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shader->GetCS()->GetBufferPointer()),
		shader->GetCS()->GetBufferSize()
	};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.NodeMask = 0;
	
	mPSOs.push_back({});
	ThrowIfFailed(device->CreateComputePipelineState(
		&psoDesc, IID_PPV_ARGS(&mPSOs.back())));
}

void ComputePipeline::SetPrevBackBuffer(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* buffer)
{
	CopyRTToMap(cmdList, buffer, mBlurMapInput[0]->GetResource());
}

void ComputePipeline::SetCurrBackBuffer(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* buffer)
{
	CopyRTToMap(cmdList, buffer, mBlurMapInput[1]->GetResource());
}

void ComputePipeline::CopyRTToMap(
	ID3D12GraphicsCommandList *cmdList,
	ID3D12Resource* source, 
	ID3D12Resource* dest)
{
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		source, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	cmdList->CopyResource(dest,source);

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		source, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void ComputePipeline::CopyMapToRT(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* rtBuffer)
{
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		rtBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));
	
	cmdList->CopyResource(rtBuffer, mBlurMapOutput->GetResource());

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		rtBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void ComputePipeline::CopyCurrentToPreviousBuffer(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapInput[0]->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapInput[1]->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->CopyResource(mBlurMapInput[0]->GetResource(), mBlurMapInput[1]->GetResource());
	
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapInput[0]->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapInput[1]->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
}

void ComputePipeline::CreateTextures(ID3D12Device* device)
{	
	for (int i = 0; i < BlurMapInputCount; i++)
	{
		mBlurMapInput[i] = std::make_unique<Texture>();
		mBlurMapInput[i]->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
		mBlurMapInput[i]->CreateTexture(device, gFrameWidth, gFrameHeight,
			1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_COMMON, nullptr);
	}
	mBlurMapOutput = std::make_unique<Texture>();
	mBlurMapOutput->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mBlurMapOutput->CreateTexture(device, gFrameWidth, gFrameHeight,
		1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON, nullptr);
}

void ComputePipeline::BuildDescriptorHeap(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(
			3,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
		IID_PPV_ARGS(&mSrvUavDescriptorHeap)));

	BuildSRVAndUAV(device);
}

void ComputePipeline::BuildSRVAndUAV(ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	auto cpuHandle = mSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < BlurMapInputCount; i++)
	{
		device->CreateShaderResourceView(mBlurMapInput[i]->GetResource(), &srvDesc, cpuHandle);
		cpuHandle.ptr += gCbvSrvUavDescriptorSize;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(mBlurMapOutput->GetResource(), nullptr, &uavDesc, cpuHandle);
}

void ComputePipeline::Dispatch(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetComputeRootSignature(mComputeRootSig);
	ID3D12DescriptorHeap* descHeap[] = { mSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeap), descHeap);

	auto gpuHandle = mSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetComputeRootDescriptorTable(0, gpuHandle);

	gpuHandle.ptr += 2 * gCbvSrvUavDescriptorSize;
	cmdList->SetComputeRootDescriptorTable(1, gpuHandle);

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapOutput->GetResource(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	cmdList->SetPipelineState(mPSOs[0].Get());

	UINT numGroupX = (UINT)(ceilf(gFrameWidth / 32.0f));
	UINT numGroupY = (UINT)(ceilf(gFrameHeight / 32.0f));

	cmdList->Dispatch(numGroupX, numGroupY, 1);

	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		mBlurMapOutput->GetResource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON));

	auto curr_time = std::chrono::system_clock::now();
	if (mSetTime + mDuration < curr_time)
	{
		CopyCurrentToPreviousBuffer(cmdList);
		mSetTime = curr_time;
	}
}