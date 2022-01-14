#include "stdafx.h"
#include "dynamicCubeRenderer.h"

#include "gameObject.h"
#include "texture.h"
#include "gameScene.h"

DynamicCubeRenderer::DynamicCubeRenderer()
{
}

DynamicCubeRenderer::~DynamicCubeRenderer()
{
}

void DynamicCubeRenderer::AppendObject(ID3D12Device* device, const std::shared_ptr<DynamicCubeMapObject> obj)
{
	mRenderObjects.push_back(obj);
	CreateTexture(device, obj);
}

void DynamicCubeRenderer::PreDraw(ID3D12GraphicsCommandList* cmdList, GameScene* scene)
{
	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		mRenderObjects[i]->PreDraw(cmdList, mTextures[i]->GetResource(), scene);
	}
}

void DynamicCubeRenderer::CreateTexture(ID3D12Device* device, const std::shared_ptr<DynamicCubeMapObject> obj)
{
	std::unique_ptr<Texture> textureCube = std::make_unique<Texture>();
	textureCube->SetDimension(D3D12_SRV_DIMENSION_TEXTURECUBE);

	D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, {0.0f,0.0f,0.0f,1.0f} };

	UINT size = (UINT)obj->GetCubeMapSize();

	textureCube->CreateTexture(
		device, size, size, RtvCounts, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue);

	mTextures.push_back(std::move(textureCube));
}

void DynamicCubeRenderer::BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex)
{
	Pipeline::BuildDescriptorHeap(device, cbvIndex, srvIndex);

	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(
			(UINT)mRenderObjects.size(),
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		IID_PPV_ARGS(&mDsvDescriptorHeap)));

	ThrowIfFailed(device->CreateDescriptorHeap(
		&Extension::DescriptorHeapDesc(
			(UINT)mRenderObjects.size() * RtvCounts,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		IID_PPV_ARGS(&mRtvDescriptorHeap)));

	for (int i = 0; i < mRenderObjects.size(); i++)
	{
		auto rtvHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		auto dsvHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		rtvHandle.ptr += i * gRtvDescriptorSize * 6;
		dsvHandle.ptr += i * gDsvDescriptorSize;

		mRenderObjects[i]->BuildDsvRtvView(device, mTextures[i]->GetResource(), rtvHandle, dsvHandle);
	}
}
