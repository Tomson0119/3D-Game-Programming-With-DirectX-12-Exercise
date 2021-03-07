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

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

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

	virtual void OnKeyUp(WPARAM btnState) override;

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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mpsoDesc;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mView = MyMathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MyMathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};

void MyBoxApp::Update(const MyGameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * cosf(mPhi);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);

	// Build the view matrix
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);

	world = XMMatrixTranslation(3.0f, 0.0f, 0.0f);
	worldViewProj = world * view * proj;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(1, objConstants);
}

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

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());

	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	int i = 0;
	for (auto obj : mBoxGeo->DrawArgs)
	{
		auto GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		GPUHandle.Offset(i, mCbvSrvUavDescriptorSize);

		mCommandList->SetGraphicsRootDescriptorTable(0, GPUHandle);

		mCommandList->DrawIndexedInstanced(obj.second.IndexCount, 1, obj.second.StartIndexLocation,
			obj.second.BaseVertexLocation, 0);
		++i;
	}

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

void MyBoxApp::OnKeyUp(WPARAM btnState)
{
	if ((int)btnState == 0x47)
	{
		mpsoDesc.RasterizerState.FillMode =
			(mpsoDesc.RasterizerState.FillMode == D3D12_FILL_MODE_SOLID) ?
			D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

		ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&mpsoDesc,
			IID_PPV_ARGS(&mPSO)));
	}
}

void MyBoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 2; // Changed
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap)));
}

void MyBoxApp::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<MyUploadBuffer<ObjectConstants>>(md3dDevice.Get(), 2, true);

	UINT objCBByteSize = MyD3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.

	for (UINT CBufIndex = 0; CBufIndex < 2; ++CBufIndex)
	{
		cbAddress += (UINT64)CBufIndex * objCBByteSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = objCBByteSize;

		auto descHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		descHandle.Offset(CBufIndex, mCbvSrvUavDescriptorSize);

		md3dDevice->CreateConstantBufferView(&cbvDesc, descHandle);
	}
}

void MyBoxApp::BuildBoxGeometry()
{
	std::array<Vertex, 13> vertices =
	{
		// Box
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Red)     }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue)    }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Green)   }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Yellow)  }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Black)   }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::White)   }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Purple)  }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::SkyBlue) }),

		// Pyramid
		Vertex({ XMFLOAT3(+0.0f, +1.0f, +0.0f), XMFLOAT4(Colors::Red)   }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) })
	};

	std::array<std::uint16_t, 54> indices =
	{
		// Box
		// Front
		0, 2, 1,
		0, 3, 2,
		// Back
		4, 5, 6,
		4, 6, 7,
		// Left
		0, 7, 3,
		0, 4, 7,
		// Right
		1, 2, 6,
		1, 6, 5,
		// Top
		3, 7, 2,
		2, 7, 6,
		// Bottom
		0, 1, 4,
		1, 5, 4,


		// Pyramid
		0, 3, 2,
		0, 2, 1,
		0, 4, 3,
		0, 1, 4,
		1, 3, 4,
		1, 2, 3
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	mBoxGeo->VertexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->IndexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry box;
	box.IndexCount = (UINT)36;
	box.StartIndexLocation = 0;
	box.BaseVertexLocation = 0;

	SubmeshGeometry pyramid;
	pyramid.IndexCount = (UINT)18;
	pyramid.StartIndexLocation = 36;
	pyramid.BaseVertexLocation = 8;

	mBoxGeo->DrawArgs["box"] = box;
	mBoxGeo->DrawArgs["pyramid"] = pyramid;
}

void MyBoxApp::BuildPSO()
{
	ZeroMemory(&mpsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	mpsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	mpsoDesc.pRootSignature = mRootSignature.Get();
	mpsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	mpsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	mpsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mpsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mpsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mpsoDesc.SampleMask = UINT_MAX;
	mpsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	mpsoDesc.NumRenderTargets = 1;
	mpsoDesc.RTVFormats[0] = mBackBufferFormat;
	mpsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	mpsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	mpsoDesc.DSVFormat = mDepthStencilFormat;

	mpsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Changed
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&mpsoDesc, IID_PPV_ARGS(&mPSO)));
}