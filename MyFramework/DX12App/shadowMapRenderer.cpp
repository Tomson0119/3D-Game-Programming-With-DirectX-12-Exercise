#include "stdafx.h"
#include "shadowMapRenderer.h"
#include "gameScene.h"

ShadowMapRenderer::ShadowMapRenderer(ID3D12Device* device, UINT width, UINT height, UINT lightCount)
	: mMapWidth(width), mMapHeight(height), mMapCount(lightCount)
{
	mViewPort = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };	
	
	for (int i = 0; i < (int)mMapCount; i++)
		mDepthCamera.push_back(std::make_unique<Camera>());
	mRtvCPUDescriptorHandles.resize(mMapCount, {});

	CreateTexture(device);
}

ShadowMapRenderer::~ShadowMapRenderer()
{
}

void ShadowMapRenderer::BuildPipeline(ID3D12Device* device, ID3D12RootSignature* rootSig, Shader* shader)
{
	auto shadowMapShader = std::make_unique<DefaultShader>(L"Shaders\\shadow.hlsl");
		
	mRasterizerDesc.DepthBias = 100000;
	mRasterizerDesc.DepthBiasClamp = 0.0f;
	mRasterizerDesc.SlopeScaledDepthBias = 1.0f;
	mBackBufferFormat = DXGI_FORMAT_R32_FLOAT;
	mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

	Pipeline::BuildPipeline(device, rootSig, shadowMapShader.get());
}

XMFLOAT4X4 ShadowMapRenderer::GetShadowTransform(int idx) const
{
	XMMATRIX view = XMLoadFloat4x4(&mDepthCamera[idx]->GetView());
	XMMATRIX proj = XMLoadFloat4x4(&mDepthCamera[idx]->GetProj());
	XMMATRIX T = XMLoadFloat4x4(&mToTexture);	
	
	XMFLOAT4X4 shadowT;
	XMStoreFloat4x4(&shadowT, view * proj * T);
	return shadowT;
}

void ShadowMapRenderer::CreateTexture(ID3D12Device* device)
{
	D3D12_CLEAR_VALUE clearValue{ DXGI_FORMAT_R32_FLOAT, {1.0f,1.0f,1.0f,1.0f} };

	for (int i = 0; i < (int)mMapCount; i++)
	{
		auto shadowMap = std::make_unique<Texture>();
		shadowMap->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);

		shadowMap->CreateTexture(
			device, mMapWidth, mMapHeight, 1, 1,
			DXGI_FORMAT_R32_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COMMON, &clearValue);

		mTextures.push_back(std::move(shadowMap));
	}
}

void ShadowMapRenderer::BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex)
{
	Pipeline::BuildDescriptorHeap(device, cbvIndex, srvIndex);

	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(1,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		IID_PPV_ARGS(&mDsvDescriptorHeap)));

	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(
			mMapCount,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		IID_PPV_ARGS(&mRtvDescriptorHeap)));

	BuildDescriptorViews(device);
}

void ShadowMapRenderer::BuildDescriptorViews(ID3D12Device* device)
{
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	mDepthBuffer = CreateTexture2DResource(
		device, mMapWidth, mMapHeight, 1, 1,
		DXGI_FORMAT_D32_FLOAT,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	mDsvCPUDescriptorHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(mDepthBuffer.Get(), &dsvDesc, mDsvCPUDescriptorHandle);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < (int)mMapCount; i++)
	{
		mRtvCPUDescriptorHandles[i] = rtvCPUHandle;
		rtvCPUHandle.ptr += gRtvDescriptorSize;

		device->CreateRenderTargetView(
			mTextures[i]->GetResource(), 
			&rtvDesc, 
			mRtvCPUDescriptorHandles[i]);
	}
}

void ShadowMapRenderer::UpdateDepthCamera(LightConstants& lightCnst)
{
	for (int i = 0; i < (int)mMapCount; i++)
	{
		XMFLOAT3 look = lightCnst.Lights[i].Direction;
		XMFLOAT3 position = Vector3::MultiplyAdd(mSunRange, look, mCenter);

		mDepthCamera[i]->LookAt(position, mCenter, XMFLOAT3(0.0f, 1.0f, 0.0f));

		switch (lightCnst.Lights[i].Type)
		{
		case DIRECTIONAL_LIGHT:
			mDepthCamera[i]->SetOrthographicLens(mCenter, mSunRange);
			break;

		case SPOT_LIGHT:
			mDepthCamera[i]->SetLens(Math::PI * 0.333f,
				(float)mMapWidth / (float)mMapHeight,
				1.0f, lightCnst.Lights[i].Range);
			break;

		case POINT_LIGHT:
			// need 6 shadow maps
			break;
		}

		mDepthCamera[i]->UpdateViewMatrix();
	}
}

void ShadowMapRenderer::PreRender(ID3D12GraphicsCommandList* cmdList, GameScene* scene)
{
	cmdList->RSSetViewports(1, &mViewPort);
	cmdList->RSSetScissorRects(1, &mScissorRect);
	cmdList->SetPipelineState(mPSO[0].Get());

	FLOAT clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	for (int i = 0; i < (int)mMapCount; i++)
	{
		cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
			mTextures[i]->GetResource(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

		cmdList->ClearRenderTargetView(mRtvCPUDescriptorHandles[i], clearValue, 0, NULL);
		cmdList->ClearDepthStencilView(mDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		cmdList->OMSetRenderTargets(1, &mRtvCPUDescriptorHandles[i], TRUE, &mDsvCPUDescriptorHandle);		

		scene->UpdateCameraConstant(i + 1, mDepthCamera[i].get());
		scene->SetCBV(cmdList, i + 1);
		RenderPipelines(cmdList);

		cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
			mTextures[i]->GetResource(), 
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_COMMON));
	}
}

void ShadowMapRenderer::RenderPipelines(ID3D12GraphicsCommandList* cmdList)
{
	for (const auto& pso : mShadowTargetPSOs)
		pso->SetAndDraw(cmdList, false, false);
}

void ShadowMapRenderer::SetShadowMapSRV(ID3D12GraphicsCommandList* cmdList, UINT srvIndex)
{
	ID3D12DescriptorHeap* descHeaps[] = { mCbvSrvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	auto gpuHandle = mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(srvIndex, gpuHandle);
}
