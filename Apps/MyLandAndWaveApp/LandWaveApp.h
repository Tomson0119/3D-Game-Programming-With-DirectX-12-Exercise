#pragma once

#include "../../Common/myd3dApp.h"
#include "../../Common/myFrameResource.h"
#include "../../Common/myGeometryGenerator.h"

#include "RenderItem.h"
#include "waves.h"

#include <DirectXColors.h>
#include <array>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class MyLandWaveApp : public MyD3DApp
{
public:

	MyLandWaveApp();
	MyLandWaveApp(const MyLandWaveApp& rhs) = delete;
	MyLandWaveApp& operator=(const MyLandWaveApp& rhs) = delete;
	~MyLandWaveApp();

	virtual bool Initialize() override;

private:

	virtual void OnResize() override;
	virtual void Update(const MyGameTimer& gt) override;
	virtual void Draw(const MyGameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void OnKeyboardInput(const MyGameTimer& gt);
	void UpdateCamera(const MyGameTimer& gt);
	void AnimateMaterials(const MyGameTimer& gt);
	void UpdateObjectCBs(const MyGameTimer& gt);
	void UpdateMaterialCBs(const MyGameTimer& gt);
	void UpdateMainPassCB(const MyGameTimer& gt);
	void UpdateWaves(const MyGameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildLandAndWavesRenderItems();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList,
		const std::vector<RenderItem*>& ritems);

	//
	// Ex 11-8.
	void DrawFullWindowQuad(ID3D12GraphicsCommandList* cmdList);
	// 
	//

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	float GetHillsHeight(float x, float z) const;
	XMFLOAT3 GetHillsNormal(float x, float z) const;

private:

	std::vector<std::unique_ptr<MyFrameResource>> mFrameResources;
	MyFrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };
	XMFLOAT4X4 mView = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MyMathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	POINT mLastMousePos = { 0, 0 };
};