#pragma once

#include "../../Common/myd3dApp.h"
#include "../../Common/myGeometryGenerator.h"
#include "../../Common/myFrameResource.h"

#include <DirectXColors.h>
#include <fstream>
#include <array>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space.
	// It defines the position, orientation, and scale of the object in the world.
	DirectX::XMFLOAT4X4 World = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MyMathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed
	// and we need to update the constant buffer.
	// When we modify object data we should set NumFramesDirty = gNumFrameResources
	// so that each frame resources gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU cbuffer corresponding to the objectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

class MyShapeApp : public MyD3DApp
{
public:

	MyShapeApp();
	MyShapeApp(const MyShapeApp& rhs) = delete;
	MyShapeApp& operator=(const MyShapeApp& rhs) = delete;
	~MyShapeApp();

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

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildShapeRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList,
		const std::vector<RenderItem*>& ritems);

	MyGeometryGenerator::MeshData CreateSkull(const std::string& fileName) const;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

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

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	PassConstants mMainPassCB;

	bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };
	XMFLOAT4X4 mView = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MyMathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 18.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	const int gNumFrameResources = 3;
	POINT mLastMousePos = { 0, 0 };
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		MyShapeApp app;
		if (!app.Initialize())
			return 0;

		return app.Run();
	}
	catch (MyDxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}