#include "stdafx.h"
#include "gameScene.h"
#include "dynamicCubeRenderer.h"

using namespace std;

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::OnResize(float aspect)
{
	if(mMainCamera)
		mMainCamera->SetLens(aspect);
	if (mPlayerCamera)
		mPlayerCamera->SetLens(aspect);
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, float aspect)
{
	mMainCamera = make_unique<Camera>();
	mMainCamera->SetLens(0.25f * Math::PI, aspect, 1.0f, 5000.0f);
	mMainCamera->SetPosition(512.0f, 200.0f, 512.0f);
	mCurrentCamera = mMainCamera.get();

	BuildRootSignature(device);
	BuildShadersAndPSOs(device, cmdList);
	BuildTextures(device, cmdList);
	BuildGameObjects(device, cmdList);
	BuildConstantBuffers(device);
	BuildDescriptorHeap(device);
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descRanges[2];
	descRanges[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	descRanges[1] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);

	D3D12_ROOT_PARAMETER parameters[4];
	parameters[0] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);  // CameraCB
	parameters[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_ALL);  // LightCB
	parameters[2] = Extension::DescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_ALL);			   // Object,  CBV
	parameters[3] = Extension::DescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_ALL);		   // Texture, SRV

	D3D12_STATIC_SAMPLER_DESC samplerDesc[4];
	samplerDesc[0] = Extension::SamplerDesc(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplerDesc[1] = Extension::SamplerDesc(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDesc[2] = Extension::SamplerDesc(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplerDesc[3] = Extension::SamplerDesc(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = Extension::RootSignatureDesc(_countof(parameters), parameters,
		_countof(samplerDesc), samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		rootSigBlob.GetAddressOf(), errorBlob.GetAddressOf()));

	ThrowIfFailed(device->CreateRootSignature(
		0, rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto terrainShader = make_unique<TerrainShader>(L"Shaders\\terrain.hlsl");
	auto billboardShader = make_unique<BillboardShader>(L"Shaders\\billboard.hlsl");
	auto normalmapShader = make_unique<DefaultShader>(L"Shaders\\normalmap.hlsl");
	auto defaultShader = make_unique<DefaultShader>(L"Shaders\\default.hlsl");
	auto dynamicCubeShader = make_unique<DefaultShader>(L"Shaders\\cubemap.hlsl");

	mPipelines[Layer::SkyBox] = make_unique<SkyboxPipeline>(device, cmdList);
	mPipelines[Layer::SkyBox]->BuildPipeline(device, mRootSignature.Get());

	mPipelines[Layer::Terrain] = make_unique<Pipeline>();
	mPipelines[Layer::Terrain]->BuildPipeline(device, mRootSignature.Get(), terrainShader.get());

	mPipelines[Layer::Billboard] = make_unique<Pipeline>();
	mPipelines[Layer::Billboard]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	mPipelines[Layer::Billboard]->SetAlphaBlending();
	mPipelines[Layer::Billboard]->BuildPipeline(device, mRootSignature.Get(), billboardShader.get());

	mPipelines[Layer::NormalMapped] = make_unique<Pipeline>();
	mPipelines[Layer::NormalMapped]->BuildPipeline(device, mRootSignature.Get(), normalmapShader.get());

	mPipelines[Layer::Default] = make_unique<Pipeline>();
	mPipelines[Layer::Default]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	/*mCubeMapRenderer = make_unique<DynamicCubeRenderer>();
	mCubeMapRenderer->BuildPipeline(device, mRootSignature.Get(), dynamicCubeShader.get());*/
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1+12);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);

	//mCubeMapRenderer->BuildConstantBuffer(device);
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 2, 3);

	//mCubeMapRenderer->BuildDescriptorHeap(device, 2, 3);
}

void GameScene::BuildTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// Terrain
	auto grassTex = make_shared<Texture>();
	grassTex->LoadTextureFromDDS(device, cmdList, L"Resources\\terrainTexture.dds");
	grassTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Terrain]->AppendTexture(grassTex);

	auto gravelTex = make_shared<Texture>();
	gravelTex->LoadTextureFromDDS(device, cmdList, L"Resources\\rocky.dds");
	gravelTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Terrain]->AppendTexture(gravelTex);

	auto roadTex = make_shared<Texture>();
	roadTex->LoadTextureFromDDS(device, cmdList, L"Resources\\road.dds");
	roadTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Terrain]->AppendTexture(roadTex);

	auto heightmapTex = make_shared<Texture>();
	heightmapTex->LoadTextureFromDDS(device, cmdList, L"Resources\\heightmap.dds");
	heightmapTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Terrain]->AppendTexture(heightmapTex);

	auto normalmapTex = make_shared<Texture>();
	normalmapTex->LoadTextureFromDDS(device, cmdList, L"Resources\\normalmap.dds");
	normalmapTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Terrain]->AppendTexture(normalmapTex);

	// Billboard
	auto grassarray = make_shared<Texture>();
	grassarray->LoadTextureFromDDS(device, cmdList, L"Resources\\grassarray.dds");
	grassarray->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mPipelines[Layer::Billboard]->AppendTexture(grassarray);

	auto treeArrayTex = make_shared<Texture>();
	treeArrayTex->LoadTextureFromDDS(device, cmdList, L"Resources\\treearray.dds");
	treeArrayTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mPipelines[Layer::Billboard]->AppendTexture(treeArrayTex);

	auto flameArrayTex = make_shared<Texture>();
	flameArrayTex->LoadTextureFromDDS(device, cmdList, L"Resources\\flamearray.dds");
	flameArrayTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mPipelines[Layer::Billboard]->AppendTexture(flameArrayTex);

	// Normalmapped
	auto brickTex = make_shared<Texture>();
	brickTex->LoadTextureFromDDS(device, cmdList, L"Resources\\brick.dds");
	brickTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brickTex);

	auto brickNormal = make_shared<Texture>();
	brickNormal->LoadTextureFromDDS(device, cmdList, L"Resources\\brickNormalmap.dds");
	brickNormal->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brickNormal);

	// Default
	auto carTex = make_shared<Texture>();
	carTex->LoadTextureFromDDS(device, cmdList, L"Resources\\CarTexture.dds");
	carTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Default]->AppendTexture(carTex);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// terrain
	auto terrain = make_shared<TerrainObject>(257, 257, XMFLOAT3(5.0f,1.0f,5.0f));
	terrain->SetSRVIndex(0);
	terrain->BuildHeightMap(L"Resources\\heightmap2.raw");
	terrain->BuildTerrainMesh(device, cmdList);
	mPipelines[Layer::Terrain]->AppendObject(terrain);

	// billboards
	auto grassBillboard = make_shared<Billboard>(2.0f, 2.0f);
	grassBillboard->SetSRVIndex(0);
	for (int i = 0; i < 20000; i++)
	{
		float pos_x = Math::RandFloat(0, terrain->GetWidth());
		float pos_z = Math::RandFloat(0, terrain->GetDepth());
		float pos_y = terrain->GetHeight(pos_x, pos_z) + 1.0f;
		if (pos_y < 0.0f) pos_y = 0.0f;

		grassBillboard->AppendBillboard(XMFLOAT3(pos_x, pos_y, pos_z));
	}
	grassBillboard->BuildMesh(device, cmdList);
	mPipelines[Layer::Billboard]->AppendObject(grassBillboard);

	auto treeBillboard = make_shared<Billboard>(5.0f, 8.0f);
	treeBillboard->SetSRVIndex(1);
	for (int i = 0; i < 10000; i++)
	{
		float pos_x = Math::RandFloat(0, terrain->GetWidth());
		float pos_z = Math::RandFloat(0, terrain->GetDepth());
		float pos_y = terrain->GetHeight(pos_x, pos_z) + 4.0f;
		if (pos_y < 0.0f) pos_y = 0.0f;

		treeBillboard->AppendBillboard(XMFLOAT3(pos_x, pos_y, pos_z));
	}
	treeBillboard->BuildMesh(device, cmdList);
	mPipelines[Layer::Billboard]->AppendObject(treeBillboard);

	mFlameBillboard = make_shared<Billboard>(2.0f, 2.0f);
	mFlameBillboard->SetSRVIndex(2);
	mFlameBillboard->AppendBillboard(XMFLOAT3(0.0f,0.0f,0.0f));
	mFlameBillboard->BuildMesh(device, cmdList);

	// boxes
	auto boxMesh = make_shared<BoxMesh>(device, cmdList, 4.0f, 4.0f, 4.0f);
	for (int i=0;i<100;i++)
	{
		float pos_x = Math::RandFloat(0, terrain->GetWidth());
		float pos_z = Math::RandFloat(0, terrain->GetDepth());
		float pos_y = terrain->GetHeight(pos_x, pos_z) + 2.0f;

		auto boxObj = make_shared<GameObject>();
		boxObj->SetMesh(boxMesh);
		boxObj->SetPosition(pos_x, pos_y, pos_z);
		boxObj->SetRotation(XMFLOAT3(0.0f, 1.0f, 0.0f), 100.0f);

		mPipelines[Layer::NormalMapped]->AppendObject(boxObj);
	}

	// car
	auto carMesh = make_shared<Mesh>();
	carMesh->LoadFromObj(device, cmdList, L"Models\\PoliceCar.obj");

	auto carObj = make_shared<TerrainPlayer>(terrain.get());
	carObj->SetMesh(carMesh);
	carObj->SetSRVIndex(0);
	carObj->SetPosition(257, 100.0f, 257);	
	mPlayer = carObj.get();
	mPipelines[Layer::Default]->AppendObject(carObj);

	float aspect = mMainCamera->GetAspect();
	mPlayerCamera.reset(mPlayer->ChangeCameraMode((int)CameraMode::THIRD_PERSON_CAMERA));
	mPlayerCamera->SetLens(0.25f * Math::PI, aspect, 1.0f, 5000.0f);
	mCurrentCamera = mPlayerCamera.get();

	/*auto sphereMesh = std::make_shared<SphereMesh>(device, cmdList, 10.0f, 20, 20);
	auto cubeSphere1 = std::make_shared<DynamicCubeMapObject>(device, cmdList, 256);
	cubeSphere1->SetMesh(sphereMesh);
	cubeSphere1->SetPosition(512.0f, 500.0f, 512.0f);
	cubeSphere1->SetMaterial(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.98f, 0.97f, 0.95f), 0.1f);
	mCubeMapRenderer->AppendObject(device, cubeSphere1);*/
}

void GameScene::PrepareCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	//mCubeMapRenderer->PreDraw(device, cmdList, this);
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState, int x, int y)
{
	if ((buttonState & MK_LBUTTON) && !GetCapture())
	{
		SetCapture(hwnd);
		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
}

void GameScene::OnProcessMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void GameScene::OnProcessMouseMove(WPARAM buttonState, int x, int y)
{
	if ((buttonState & MK_LBUTTON) && GetCapture())
	{
		float dx = static_cast<float>(x - mLastMousePos.x);
		float dy = static_cast<float>(y - mLastMousePos.y);

		mLastMousePos.x = x;
		mLastMousePos.y = y;

		if (mCurrentCamera == mMainCamera.get())
		{
			mCurrentCamera->RotateY(0.25f * dx);
			mCurrentCamera->Pitch(0.25f * dy);
		}
		else
		{
			mPlayer->RotateY(0.25f * dx);
		}
	}
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == 'E')
		{
			if (mCurrentCamera == mPlayerCamera.get())
			{
				mMainCamera->SetPosition(mPlayerCamera->GetPosition());
				mMainCamera->LookAt(mMainCamera->GetPosition(),
					mPlayer->GetPosition(), XMFLOAT3(0.0f,1.0f,0.0f));
				mCurrentCamera = mMainCamera.get();
			}
			else
			{
				mCurrentCamera = mPlayerCamera.get();
			}
		}
		break;
	}
}

void GameScene::OnPreciseKeyInput(float elapsed)
{
	bool player_active = (mCurrentCamera == mPlayerCamera.get());

	if (GetAsyncKeyState('W') & 0x8000)
	{
		if (player_active)
			mPlayer->Walk(500.0f * elapsed);
		else
			mCurrentCamera->Walk(100.0f * elapsed);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		if (player_active)
			mPlayer->Strafe(-500.0f * elapsed);
		else
			mCurrentCamera->Strafe(-100.0f * elapsed);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		if (player_active)
			mPlayer->Walk(-500.0f * elapsed);
		else
			mCurrentCamera->Walk(-100.0f * elapsed);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		if (player_active)
			mPlayer->Strafe(500.0f * elapsed);
		else
			mCurrentCamera->Strafe(100.0f * elapsed);
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		if (!player_active)
			mCurrentCamera->Upward(100.0f * elapsed);
	}
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	{
		if (!player_active)
			mCurrentCamera->Upward(-100.0f * elapsed);
	}
}

void GameScene::Update(ID3D12Device* device, const GameTimer& timer)
{
	float elapsed = timer.ElapsedTime();

	OnPreciseKeyInput(elapsed);

	mCurrentCamera->Update(elapsed);

	for (const auto& [_, pso] : mPipelines)
		pso->Update(elapsed, mCurrentCamera);

	CollisionProcess(device);
	DeleteTimeOverBillboards(device);

	//mCubeMapRenderer->Update(timer.ElapsedTime(), mCurrentCamera);
}

void GameScene::UpdateCameraConstant(int idx, Camera* camera)
{
	// 카메라로부터 상수를 받는다.
	mCameraCB->CopyData(idx, camera->GetConstants());
}

void GameScene::UpdateConstants()
{
	UpdateCameraConstant(0, mCurrentCamera);

	// 광원과 관련된 상수버퍼를 초기화 및 업데이트한다.
	LightConstants lightCnst;
	lightCnst.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

	lightCnst.Lights[0].Diffuse = { 0.5f, 0.5f, 0.5f };
	lightCnst.Lights[0].Direction = { -1.0f, 1.0f, -1.0f };

	lightCnst.Lights[1].Diffuse = { 0.15f, 0.15f, 0.15f };
	lightCnst.Lights[1].Direction = { 0.0f, 1.0f, 1.0f };

	lightCnst.Lights[2].Diffuse = { 0.35f, 0.35f, 0.35f };
	lightCnst.Lights[2].Direction = { 1.0f, 1.0f, -1.0f };

	mLightCB->CopyData(0, lightCnst);

	for (const auto& [_, pso] : mPipelines)
		pso->UpdateConstants();

	//mCubeMapRenderer->UpdateConstants();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex)
{	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress(cameraCBIndex));
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress(0));
	
	for (const auto& [layer, pso] : mPipelines)
		if(layer != Layer::Billboard) pso->SetAndDraw(cmdList);

	//mCubeMapRenderer->SetAndDraw(cmdList);
}

void GameScene::CollisionProcess(ID3D12Device* device)
{
	const vector<shared_ptr<GameObject>>& boxes = mPipelines[Layer::NormalMapped]->GetRenderObjects();
	
	const float big_radius = 20.0f;
	for (int i = (int)boxes.size() - 1; i >= 0; i--)
	{
		float dist = Vector3::Length(Vector3::Subtract(boxes[i]->GetPosition(), mPlayer->GetPosition()));
		if (dist < big_radius)
		{
			const BoundingOrientedBox& box_bound = boxes[i]->GetBoundingBox();
			const BoundingOrientedBox& player_bound = mPlayer->GetBoundingBox();

			if (box_bound.Intersects(player_bound))
			{
				CreateAndAppendFlameBillboard(device, boxes[i].get());
				mPipelines[Layer::NormalMapped]->DeleteObject(i);
			}
		}
	}
	mPipelines[Layer::NormalMapped]->ResetPipeline(device);
}

void GameScene::CreateAndAppendFlameBillboard(ID3D12Device* device, GameObject* box)
{
	static XMFLOAT3 Direction[9] = {
		XMFLOAT3( 1.0f, 0.0f,  0.0f),
		XMFLOAT3(-1.0f, 0.0f,  0.0f),
		XMFLOAT3( 0.0f, 1.0f,  0.0f),
		XMFLOAT3( 0.0f, 0.0f,  1.0f),
		XMFLOAT3( 0.0f, 0.0f, -1.0f),
		XMFLOAT3( 1.0f, 1.0f,  1.0f),
		XMFLOAT3( 1.0f, 1.0f, -1.0f),
		XMFLOAT3(-1.0f, 1.0f,  1.0f),
		XMFLOAT3(-1.0f, 1.0f, -1.0f)
	};

	int offset = (int)mPipelines[Layer::Billboard]->GetRenderObjects().size();
	for (int i = 0; i < 9; i++)
	{
		shared_ptr<Billboard> newFlame = make_shared<Billboard>(*mFlameBillboard);
		newFlame->SetPosition(box->GetPosition());
		newFlame->SetMovement(Direction[i], 1.0f);
		newFlame->SetDurationTime(2500ms);
		mPipelines[Layer::Billboard]->AppendObject(newFlame);
		
		mAllFlameBillboards.push_back({ offset + i, newFlame.get() });
	}
	mPipelines[Layer::Billboard]->ResetPipeline(device);
}

void GameScene::DeleteTimeOverBillboards(ID3D12Device* device)
{
	bool flag = false;
	for (int i = (int)mAllFlameBillboards.size()-1; i >= 0; i--)
	{
		if (mAllFlameBillboards[i].second->IsTimeOver(std::chrono::steady_clock::now()))
		{
			flag = true;
			mPipelines[Layer::Billboard]->DeleteObject(mAllFlameBillboards[i].first);
			mAllFlameBillboards.erase(mAllFlameBillboards.begin() + i);
		}
	}
	if (flag) 
	{
		mPipelines[Layer::Billboard]->ResetPipeline(device);
	}
}


