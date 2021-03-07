#include <crtdbg.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <memory>
#include <array>

#include "myd3dApp.h"
#include "MyUploadBuffer.h"
#include "MyMathHelper.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


//************** Chapter 6 exercise 2 **************//
struct VPosData
{
	XMFLOAT3 Pos;
};

struct VColorData
{
	XMFLOAT4 Color;
};
//**************************************************//

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MyMathHelper::Identity4x4();
};

class MyBoxApp : public MyD3DApp
{
public:
	MyBoxApp();
	MyBoxApp(const MyBoxApp& rhs) = delete;
	MyBoxApp& operator=(const MyBoxApp& rhs) = delete;
	~MyBoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const MyGameTimer& gt) override;
	virtual void Draw(const MyGameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaderAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	XMVECTORF32 mBgColor = Colors::SkyBlue;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<MyUploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mView = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MyMathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;

	//************** Chapter 6 exercise 2 **************//
	std::array<D3D12_VERTEX_BUFFER_VIEW, 2> mvBufferViews;

	ComPtr<ID3DBlob> mvPosBufferCPU = nullptr;
	ComPtr<ID3D12Resource> mvPosBufferGPU = nullptr;
	ComPtr<ID3D12Resource> mvPosBufferUploader = nullptr;

	ComPtr<ID3DBlob> mvColorBufferCPU = nullptr;
	ComPtr<ID3D12Resource> mvColorBufferGPU = nullptr;
	ComPtr<ID3D12Resource> mvColorBufferUploader = nullptr;
	//**************************************************//
};

void MyBoxApp::Draw(const MyGameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists
	// have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added
	// to the command queue via ExecuteCommandList.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	// Set the viewport and scissor rect. 
	// This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(),
		mBgColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	//mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());

	//************** Chapter 6 exercise 2 **************//
	mCommandList->IASetVertexBuffers(0, 1, &mvBufferViews[0]);
	mCommandList->IASetVertexBuffers(1, 1, &mvBufferViews[1]);

	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete. This waiting is inefficient and
	// is done for simplicity. Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void MyBoxApp::BuildShaderAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = MyD3DUtil::CompileShader(L"color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = MyD3DUtil::CompileShader(L"color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		/*{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}*/

		//************** Chapter 6 exercise 2 **************//
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		//**************************************************//
	};
}

void MyBoxApp::BuildBoxGeometry()
{
	/*std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Black)   }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::White)   }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Green)   }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue)    }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Red)     }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Purple)  }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  })
	};*/

	//************** Chapter 6 exercise 2 **************//
	std::array<VPosData, 8> vPosData =
	{
		VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) })
	};

	std::array<VColorData, 8> vColorData =
	{
		VColorData({ XMFLOAT4(Colors::Black)   }),
		VColorData({ XMFLOAT4(Colors::Red)     }),
		VColorData({ XMFLOAT4(Colors::Green)   }),
		VColorData({ XMFLOAT4(Colors::White)   }),
		VColorData({ XMFLOAT4(Colors::Purple)  }),
		VColorData({ XMFLOAT4(Colors::Blue)    }),
		VColorData({ XMFLOAT4(Colors::Yellow)  }),
		VColorData({ XMFLOAT4(Colors::Magenta) })
	};
	//**************************************************//

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	// const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	//************** Chapter 6 exercise 2 **************//
	const UINT vpbByteSize = (UINT)vPosData.size() * sizeof(VPosData);
	const UINT vcbByteSize = (UINT)vColorData.size() * sizeof(VColorData);
	//**************************************************//
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	// ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	// CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	//************** Chapter 6 exercise 2 **************//
	ThrowIfFailed(D3DCreateBlob(vpbByteSize, &mvPosBufferCPU));
	CopyMemory(mvPosBufferCPU->GetBufferPointer(), vPosData.data(), vpbByteSize);

	mvPosBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vPosData.data(), vpbByteSize, mvPosBufferUploader);

	ThrowIfFailed(D3DCreateBlob(vcbByteSize, &mvColorBufferCPU));
	CopyMemory(mvColorBufferCPU->GetBufferPointer(), vColorData.data(), vcbByteSize);

	mvColorBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vColorData.data(), vcbByteSize, mvColorBufferUploader);
	//**************************************************//

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->IndexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	//mBoxGeo->VertexByteStride = sizeof(Vertex);
	//mBoxGeo->VertexBufferByteSize = vbByteSize;

	//************** Chapter 6 exercise 2 **************//
	mvBufferViews =
	{
		// Vertex Pos Buffer
		D3D12_VERTEX_BUFFER_VIEW({ mvPosBufferGPU->GetGPUVirtualAddress(),
			vpbByteSize, sizeof(VPosData) }),
			// Vertex Color Buffer
			D3D12_VERTEX_BUFFER_VIEW({ mvColorBufferGPU->GetGPUVirtualAddress(),
				vcbByteSize, sizeof(VColorData) })
	};
	//**************************************************//

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}