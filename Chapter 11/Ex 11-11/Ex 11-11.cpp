#include "Ex 11-11.h"
#include "../../Common/DDSTextureLoader.h"

MyStencilDemo::MyStencilDemo()
	: MyD3DApp()
{
}

MyStencilDemo::~MyStencilDemo()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool MyStencilDemo::Initialize()
{
	if (!MyD3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRoomGeometry();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void MyStencilDemo::OnResize()
{
	MyD3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MyMathHelper::PI,
		AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void MyStencilDemo::Update(const MyGameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		if (eventHandle != 0) {
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateReflectedPassCB(gt);
}

void MyStencilDemo::Draw(const MyGameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists 
	// have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue 
	// via ExecuteCommandList. Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
	
	// Draw opaque items--floors, walls, skull.
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// Mark the visible mirror pixels in the stencil buffer with the value 1
	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPSOs["mirrors"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Mirrors]);

	//// Draw the reflection into the mirror only
	//// (only for pixels where the stencil buffer is 1).
	//// Note that we must supply a different per-pass constant buffer
	//// one with the light reflected.	
	UINT passCBByteSize = MyD3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	mCommandList->SetPipelineState(mPSOs["reflections"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Reflected]);

	//// Restore main pass constants and stencil ref.
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
	mCommandList->OMSetStencilRef(0);

	//// Draw mirror with transparency so reflection blends through.
	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	//// Draw shadows
	mCommandList->SetPipelineState(mPSOs["shadow"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Shadow]);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void MyStencilDemo::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(m_hwnd);
}

void MyStencilDemo::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MyStencilDemo::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(-x + mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(-y + mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MyMathHelper::Clamp(mPhi, 0.1f, MyMathHelper::PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MyMathHelper::Clamp(mRadius, 5.0f, mRadius * 3.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MyStencilDemo::OnKeyboardInput(const MyGameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('A') & 0x8000)
		mSkullTranslation.z += 1.0f * dt;

	if (GetAsyncKeyState('D') & 0x8000)
		mSkullTranslation.z -= 1.0f * dt;

	if (GetAsyncKeyState('W') & 0x8000)
		mSkullTranslation.x += 1.0f * dt;

	if (GetAsyncKeyState('S') & 0x8000)
		mSkullTranslation.x -= 1.0f * dt;

	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
		mSkullTranslation.y -= 1.0f * dt;

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		mSkullTranslation.y += 1.0f * dt;

	// Don't let user move below ground plane and mirror plane.	
	mSkullTranslation.y = MyMathHelper::Max(mSkullTranslation.y, 0.0f);
	mSkullTranslation.z = MyMathHelper::Min(mSkullTranslation.z, -1.5f);

	// Update the new world matrix.
	XMMATRIX skullRotate = XMMatrixRotationY(0.5f * MyMathHelper::PI);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(mSkullTranslation.x,
		mSkullTranslation.y, mSkullTranslation.z);
	XMMATRIX skullWorld = skullRotate * skullScale * skullOffset;
	XMStoreFloat4x4(&mSkullRitem->World, skullWorld);

	// Update reflection world matrix.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane.
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&mReflectedSkullRitem->World, skullWorld * R);

	// Update shadow world matrix.
	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane.
	XMVECTOR toMainLight = -XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
	XMStoreFloat4x4(&mShadowedSkullRitem->World, skullWorld * S * shadowOffsetY);

	mSkullRitem->NumFramesDirty = gNumFrameResources;
	mReflectedSkullRitem->NumFramesDirty = gNumFrameResources;
	mShadowedSkullRitem->NumFramesDirty = gNumFrameResources;
}

void MyStencilDemo::UpdateCamera(const MyGameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void MyStencilDemo::AnimateMaterials(const MyGameTimer& gt)
{
	
}

void MyStencilDemo::UpdateObjectCBs(const MyGameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void MyStencilDemo::UpdateMaterialCBs(const MyGameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void MyStencilDemo::UpdateMainPassCB(const MyGameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f,0.25f,0.25f,1.0f };

	// For 3-points lighting.
	XMVECTOR mainLightDir = -MyMathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMVECTOR backLightDir = -MyMathHelper::SphericalToCartesian(1.0f, mSunTheta - 1.5f * XM_PIDIV2, mSunPhi);
	XMVECTOR sideLightDir = -MyMathHelper::SphericalToCartesian(1.0f, mSunTheta + XM_PIDIV2, mSunPhi);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, mainLightDir);
	mMainPassCB.Lights[0].Strength = XMFLOAT3(0.5f, 0.5f, 0.5f);

	XMStoreFloat3(&mMainPassCB.Lights[1].Direction, backLightDir);
	mMainPassCB.Lights[1].Strength = XMFLOAT3(0.15f, 0.15f, 0.15f);

	XMStoreFloat3(&mMainPassCB.Lights[2].Direction, sideLightDir);
	mMainPassCB.Lights[2].Strength = XMFLOAT3(0.35f, 0.35f, 0.35f);

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void MyStencilDemo::UpdateReflectedPassCB(const MyGameTimer& gt)
{
	mReflectedPassCB = mMainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	// Reflect the lighting.
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}

	// Reflected pass stored in index 1
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}

void MyStencilDemo::LoadTextures()
{
	const std::vector<std::string> texName =
	{
		"bricksTex",
		"checkboardTex",
		"iceTex",
		"whiteTex"
	};

	const std::vector<std::wstring> texFileName =
	{
		L"../../Textures/bricks3.dds",
		L"../../Textures/checkboard.dds",
		L"../../Textures/ice.dds",
		L"../../Textures/white1x1.dds"
	};

	for (size_t i = 0; i < texFileName.size(); ++i)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = texName[i];
		tex->Filename = texFileName[i];
		ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), tex->Filename.c_str(),
			tex->Resource, tex->UploadHeap));

		mTextures[tex->Name] = std::move(tex);
	}
}

void MyStencilDemo::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Create root CBV.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Create a root signature with a single slot which points to 
	// a descriptor range consisting of a single constant buffer.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA(static_cast<LPCSTR>(errorBlob->GetBufferPointer()));
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void MyStencilDemo::BuildDescriptorHeaps()
{
	const std::vector<ComPtr<ID3D12Resource>> resources =
	{
		mTextures["bricksTex"]->Resource,		// 0
		mTextures["checkboardTex"]->Resource,	// 1
		mTextures["iceTex"]->Resource,			// 2
		mTextures["whiteTex"]->Resource			// 3
	};

	// Create the SRV heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = resources.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	// Fill out the heap with actual descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 0th descriptor.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (auto e : resources)
	{
		srvDesc.Format = e->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = e->GetDesc().MipLevels;

		md3dDevice->CreateShaderResourceView(e.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void MyStencilDemo::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestedDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = MyD3DUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = MyD3DUtil::CompileShader(L"Shaders\\default.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = MyD3DUtil::CompileShader(L"Shaders\\default.hlsl", alphaTestedDefines, "PS", "ps_5_0");
	
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void MyStencilDemo::BuildRoomGeometry()
{
	// Create and specify geometry. For this sample we draw a floor
	// and a wall with a mirror on it. We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//	 |-----------------|
	//   |----|-------|----|
	//   |Wall| Mirror|Wall|
	//   |    |       |    |
	//   /-----------------/
	//  /     Floor       /
	// /-----------------/

	std::array<Vertex, 20> vertices =
	{
		// Floor : Observe we tile texture coordinates.
		Vertex(-5.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0
		Vertex(-5.0f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f), // 1
		Vertex(+5.0f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f), // 2
		Vertex(+5.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f), // 3

		// Wall : Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-5.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-5.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f), // 5
		Vertex(-4.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f), // 6
		Vertex(-4.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f), // 7

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f),  // 8
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),  // 9
		Vertex(5.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.25f, 0.0f),  // 10
		Vertex(5.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.25f, 2.0f),  // 11

		Vertex(-5.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-5.0f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f), // 13
		Vertex(+5.0f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 5.0f, 0.0f), // 14
		Vertex(+5.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 5.0f, 1.0f), // 15

		// Mirror
		Vertex(-4.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.6f),  // 16
		Vertex(-4.0f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),  // 17
		Vertex(+2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),  // 18
		Vertex(+2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.6f),  // 19
	};

	std::array<std::uint16_t, 30> indices =
	{
		// Floor
		0, 1, 2,
		0, 2, 3,

		// Wall
		4, 5, 6,
		4, 6, 7,
		8, 9, 10,
		8, 10, 11,
		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
	};

	SubmeshGeometry floorSubmesh;
	floorSubmesh.IndexCount = 6;
	floorSubmesh.BaseVertexLocation = 0;
	floorSubmesh.StartIndexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = 18;
	wallSubmesh.BaseVertexLocation = 0;
	wallSubmesh.StartIndexLocation = 6;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.BaseVertexLocation = 0;
	mirrorSubmesh.StartIndexLocation = 24;

	const UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(Vertex);
	const UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "roomGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	geo->DrawArgs["floor"] = std::move(floorSubmesh);
	geo->DrawArgs["wall"] = std::move(wallSubmesh);
	geo->DrawArgs["mirror"] = std::move(mirrorSubmesh);

	mGeometries[geo->Name] = std::move(geo);
}

void MyStencilDemo::BuildSkullGeometry()
{
	std::ifstream fileIn("Models/skull.txt");

	if (!fileIn)
	{
		MessageBox(NULL, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vCount = 0;
	UINT tCount = 0;

	std::string ignore;

	fileIn >> ignore >> vCount;
	fileIn >> ignore >> tCount;
	fileIn >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vCount);
	for (UINT i = 0; i < vCount; ++i)
	{
		fileIn >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fileIn >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		// Model does not have texture coordinates, so just zero them out.
		vertices[i].TexC = { 0.0f, 0.0f };
	}

	fileIn >> ignore;
	fileIn >> ignore;
	fileIn >> ignore;

	std::vector<std::uint16_t> indices((size_t)3 * tCount);
	for (size_t i = 0; i < tCount; ++i)
	{
		fileIn >> indices[i * 3] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}
	fileIn.close();

	const UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(Vertex);
	const UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = MyD3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;

	geo->DrawArgs["skull"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyStencilDemo::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;


	// PSO for opaque objects.
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	// PSO for transparency blend objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));


	// PSO for marking stencil mirrors.
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	D3D12_DEPTH_STENCIL_DESC mirrorDSS;
	mirrorDSS.DepthEnable = true;
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true;
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff;

	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	
	// We are not rendering backfacing polygons, so these settings do not matter.
	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorPsoDesc = opaquePsoDesc;
	markMirrorPsoDesc.BlendState = mirrorBlendState;
	markMirrorPsoDesc.DepthStencilState = mirrorDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&markMirrorPsoDesc, IID_PPV_ARGS(&mPSOs["mirrors"])));

	
	// PSO for stencil reflections.
	D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
	reflectionsDSS.DepthEnable = true;
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionsDSS.StencilEnable = true;
	reflectionsDSS.StencilReadMask = 0xff;
	reflectionsDSS.StencilWriteMask = 0xff;

	reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = opaquePsoDesc;
	drawReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
	drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs["reflections"])));

	D3D12_DEPTH_STENCIL_DESC shadowDDS;
	shadowDDS.DepthEnable = true;
	shadowDDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowDDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowDDS.StencilEnable = true;
	shadowDDS.StencilReadMask = 0xff;
	shadowDDS.StencilWriteMask = 0xff;

	shadowDDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	shadowDDS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDDS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDDS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDDS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoDesc;
	shadowPsoDesc.DepthStencilState = shadowDDS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&shadowPsoDesc, IID_PPV_ARGS(mPSOs["shadow"].GetAddressOf())));
}

void MyStencilDemo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<MyFrameResource>(md3dDevice.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void MyStencilDemo::BuildMaterials()
{
	auto bricks = std::make_unique<Material>();
	bricks->Name = "bricks";
	bricks->MatCBIndex = 0;
	bricks->DiffuseSrvHeapIndex = 0;
	bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks->Roughness = 0.25f;
	mMaterials[bricks->Name] = std::move(bricks);

	auto checkboard = std::make_unique<Material>();
	checkboard->Name = "checkboard";
	checkboard->MatCBIndex = 1;
	checkboard->DiffuseSrvHeapIndex = 1;
	checkboard->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	checkboard->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	checkboard->Roughness = 0.3f;
	mMaterials[checkboard->Name] = std::move(checkboard);

	auto iceMirror = std::make_unique<Material>();
	iceMirror->Name = "iceMirror";
	iceMirror->MatCBIndex = 2;
	iceMirror->DiffuseSrvHeapIndex = 2;
	iceMirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	iceMirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	iceMirror->Roughness = 0.5f;
	mMaterials[iceMirror->Name] = std::move(iceMirror);

	auto skull = std::make_unique<Material>();
	skull->Name = "skull";
	skull->MatCBIndex = 3;
	skull->DiffuseSrvHeapIndex = 3;
	skull->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skull->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skull->Roughness = 0.3f;
	mMaterials[skull->Name] = std::move(skull);

	auto shadow = std::make_unique<Material>();
	shadow->Name = "shadow";
	shadow->MatCBIndex = 4;
	shadow->DiffuseSrvHeapIndex = 3;
	shadow->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadow->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadow->Roughness = 0.0f;
	mMaterials[shadow->Name] = std::move(shadow);
}

void MyStencilDemo::BuildRenderItems()
{
	auto floorRitem = std::make_unique<RenderItem>();
	floorRitem->ObjCBIndex = 0;
	floorRitem->Mat = mMaterials["checkboard"].get();
	floorRitem->Geo = mGeometries["roomGeo"].get();
	floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->IndexCount = floorRitem->Geo->DrawArgs["floor"].IndexCount;
	floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs["floor"].BaseVertexLocation;
	floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs["floor"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(floorRitem.get());

	auto wallRitem = std::make_unique<RenderItem>();
	wallRitem->ObjCBIndex = 1;
	wallRitem->Mat = mMaterials["bricks"].get();
	wallRitem->Geo = mGeometries["roomGeo"].get();
	wallRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRitem->IndexCount = wallRitem->Geo->DrawArgs["wall"].IndexCount;
	wallRitem->BaseVertexLocation = wallRitem->Geo->DrawArgs["wall"].BaseVertexLocation;
	wallRitem->StartIndexLocation = wallRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(wallRitem.get());

	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->ObjCBIndex = 2;
	skullRitem->Mat = mMaterials["skull"].get();
	skullRitem->Geo = mGeometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	mSkullRitem = skullRitem.get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

	// Reflected skulll will have different world matrix
	// so it needs to be its own render item.
	auto reflectedSkullRitem = std::make_unique<RenderItem>();
	*reflectedSkullRitem = *skullRitem;
	reflectedSkullRitem->ObjCBIndex = 3;
	mReflectedSkullRitem = reflectedSkullRitem.get();
	mRitemLayer[(int)RenderLayer::Reflected].push_back(reflectedSkullRitem.get());

	// Ex 11-11
	// Reflected floor will have different world matrix.
	// so it needs to be its own render item.
	auto reflectedFloorRitem = std::make_unique<RenderItem>();	
	*reflectedFloorRitem = *floorRitem;
	XMMATRIX R = XMMatrixReflect(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)); // xy plane
	XMStoreFloat4x4(&reflectedFloorRitem->World, XMLoadFloat4x4(&floorRitem->World) * R);
	reflectedFloorRitem->ObjCBIndex = 4;
	mRitemLayer[(int)RenderLayer::Reflected].push_back(reflectedFloorRitem.get());
	//
	//

	// Shadowed skull will have different world matrix.
	// so it needs to be its own render item.
	auto shadowedSkullRitem = std::make_unique<RenderItem>();
	*shadowedSkullRitem = *skullRitem;
	shadowedSkullRitem->ObjCBIndex = 5;
	shadowedSkullRitem->Mat = mMaterials["shadow"].get();
	mShadowedSkullRitem = shadowedSkullRitem.get();
	mRitemLayer[(int)RenderLayer::Shadow].push_back(shadowedSkullRitem.get());

	auto mirrorRitem = std::make_unique<RenderItem>();
	mirrorRitem->ObjCBIndex = 6;
	mirrorRitem->Mat = mMaterials["iceMirror"].get();
	mirrorRitem->Geo = mGeometries["roomGeo"].get();
	mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Mirrors].push_back(mirrorRitem.get());
	mRitemLayer[(int)RenderLayer::Transparent].push_back(mirrorRitem.get());

	mAllRitems.push_back(std::move(floorRitem));
	mAllRitems.push_back(std::move(wallRitem));
	mAllRitems.push_back(std::move(skullRitem));
	mAllRitems.push_back(std::move(reflectedSkullRitem));
	mAllRitems.push_back(std::move(reflectedFloorRitem)); // Ex 11-11
	mAllRitems.push_back(std::move(shadowedSkullRitem));
	mAllRitems.push_back(std::move(mirrorRitem));
}

void MyStencilDemo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = MyD3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = MyD3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);
	
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress()
			+ (UINT64)ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress()
			+ (UINT64)ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(2, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> MyStencilDemo::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers. So just define them
	// all up front and keep them available as part of the root signature.

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}
