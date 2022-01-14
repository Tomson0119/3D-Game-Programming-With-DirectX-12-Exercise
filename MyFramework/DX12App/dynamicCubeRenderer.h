#pragma once

#include "pipeline.h"

class DynamicCubeMapObject;
class Texture;
class GameScene;

class DynamicCubeRenderer : public Pipeline
{
public :
	DynamicCubeRenderer();
	virtual ~DynamicCubeRenderer();

	virtual void BuildDescriptorHeap(ID3D12Device* device, UINT cbvIndex, UINT srvIndex);

	void AppendObject(ID3D12Device* device, const std::shared_ptr<DynamicCubeMapObject> obj);
	void PreDraw(ID3D12GraphicsCommandList* cmdList, GameScene* scene);

private:
	void CreateTexture(ID3D12Device* device, const std::shared_ptr<DynamicCubeMapObject> obj);

private:
	static const int RtvCounts = 6;

	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap;
};