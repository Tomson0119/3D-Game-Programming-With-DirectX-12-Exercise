#include "stdafx.h"
#include "gameScene.h"
#include "dynamicCubeRenderer.h"
#include "shadowMapRenderer.h"

using namespace std;

GameScene::GameScene()
{
	mPrevTime = std::chrono::high_resolution_clock::now();
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
	mMainCamera->SetPosition(257, 200.0f, 257);
	mCurrentCamera = mMainCamera.get();

	mMainLight.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mMainLight.Lights[0].SetInfo(
		XMFLOAT3(0.5f, 0.5f, 0.5f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(-1.0f, 1.0f, -1.0f),
		3000.0f, DIRECTIONAL_LIGHT);
	mMainLight.Lights[1].SetInfo(
		XMFLOAT3(0.15f, 0.15f, 0.15f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 1.0f),
		3000.0f, DIRECTIONAL_LIGHT);
	mMainLight.Lights[2].SetInfo(
		XMFLOAT3(0.35f, 0.35f, 0.35f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(1.0f, 1.0f, -1.0f),
		3000.0f, DIRECTIONAL_LIGHT);

	BuildRootSignature(device);
	BuildComputeRootSignature(device);
	BuildShadersAndPSOs(device, cmdList);
	BuildTextures(device, cmdList);
	BuildGameObjects(device, cmdList);
	BuildConstantBuffers(device);
	BuildDescriptorHeap(device);
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descRanges[3];
	descRanges[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3);
	descRanges[1] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);
	descRanges[2] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

	D3D12_ROOT_PARAMETER parameters[6];
	parameters[0] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);    // CameraCB
	parameters[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_ALL);    // LightCB
	parameters[2] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 2, D3D12_SHADER_VISIBILITY_ALL);    // GameInfoCB
	parameters[3] = Extension::DescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_ALL);			     // Object,  CBV
	parameters[4] = Extension::DescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_ALL);				 // Texture, SRV
	parameters[5] = Extension::DescriptorTable(1, &descRanges[2], D3D12_SHADER_VISIBILITY_ALL);				 // ShadowMap 																	   
    
	D3D12_STATIC_SAMPLER_DESC samplerDesc[5];
	samplerDesc[0] = Extension::SamplerDesc(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplerDesc[1] = Extension::SamplerDesc(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDesc[2] = Extension::SamplerDesc(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	samplerDesc[3] = Extension::SamplerDesc(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDesc[4] = Extension::SamplerDesc(4,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = Extension::RootSignatureDesc(_countof(parameters), parameters,
		_countof(samplerDesc), samplerDesc, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT);

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

void GameScene::BuildComputeRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descRanges[2];
	descRanges[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
	descRanges[1] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	D3D12_ROOT_PARAMETER parameters[2];
	parameters[0] = Extension::DescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_ALL);    // Inputs
	parameters[1] = Extension::DescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_ALL);    // Output																   

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = Extension::RootSignatureDesc(_countof(parameters), parameters,
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		rootSigBlob.GetAddressOf(), errorBlob.GetAddressOf()));

	ThrowIfFailed(device->CreateRootSignature(
		0, rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&mComputeRootSignature)));
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto terrainShader = make_unique<TerrainShader>(L"Shaders\\terrain.hlsl");
	auto billboardShader = make_unique<BillboardShader>(L"Shaders\\billboard.hlsl");
	auto normalmapShader = make_unique<DefaultShader>(L"Shaders\\normalmap.hlsl");
	auto defaultShader = make_unique<DefaultShader>(L"Shaders\\default.hlsl");
	auto shadowDebugShader = make_unique<DefaultShader>(L"Shaders\\shadowDebug.hlsl");

	mPipelines[Layer::SkyBox] = make_unique<SkyboxPipeline>(device, cmdList);
	mPipelines[Layer::SkyBox]->BuildPipeline(device, mRootSignature.Get());

	mPipelines[Layer::Terrain] = make_unique<Pipeline>();
	mPipelines[Layer::Terrain]->SetWiredFrame(true);
	mPipelines[Layer::Terrain]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	mPipelines[Layer::Terrain]->BuildPipeline(device, mRootSignature.Get(), terrainShader.get());

	mPipelines[Layer::Billboard] = make_unique<Pipeline>();
	mPipelines[Layer::Billboard]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	mPipelines[Layer::Billboard]->SetAlphaBlending();
	mPipelines[Layer::Billboard]->BuildPipeline(device, mRootSignature.Get(), billboardShader.get());

	mPipelines[Layer::NormalMapped] = make_unique<Pipeline>();
	mPipelines[Layer::NormalMapped]->BuildPipeline(device, mRootSignature.Get(), normalmapShader.get());

	mPipelines[Layer::Default] = make_unique<Pipeline>();
	mPipelines[Layer::Default]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	mPipelines[Layer::Mirror] = make_unique<Pipeline>();
	mPipelines[Layer::Mirror]->SetStencilOp(
		1, D3D12_DEPTH_WRITE_MASK_ZERO,
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS, 0);
	mPipelines[Layer::Mirror]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	mPipelines[Layer::Reflected] = make_unique<Pipeline>();
	mPipelines[Layer::Reflected]->SetCullClockwise();
	mPipelines[Layer::Reflected]->SetStencilOp(
		1, D3D12_DEPTH_WRITE_MASK_ALL,
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_COLOR_WRITE_ENABLE_ALL);
	mPipelines[Layer::Reflected]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	mPipelines[Layer::Transparent] = make_unique<Pipeline>();
	mPipelines[Layer::Transparent]->SetAlphaBlending();
	mPipelines[Layer::Transparent]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	mPipelines[Layer::ShadowDebug] = make_unique<Pipeline>();
	mPipelines[Layer::ShadowDebug]->BuildPipeline(device, mRootSignature.Get(), shadowDebugShader.get());
	
	mPipelines[Layer::Particle] = make_unique<StreamOutputPipeline>();
	mPipelines[Layer::Particle]->BuildPipeline(device, mRootSignature.Get());

	mShadowMapRenderer = make_unique<ShadowMapRenderer>(device, 4096, 4096, 1);
	mShadowMapRenderer->SetSunRange(80.0f);
	mShadowMapRenderer->SetCenter(mRoomCenter);
	mShadowMapRenderer->AppendTargetPipeline(mPipelines[Layer::NormalMapped].get());
	mShadowMapRenderer->AppendTargetPipeline(mPipelines[Layer::Default].get());
	mShadowMapRenderer->BuildPipeline(device, mRootSignature.Get());

	auto computeShader = make_unique<ComputeShader>(L"Shaders\\blur.hlsl");
	mComputePipeline = make_unique<ComputePipeline>(device);
	mComputePipeline->BuildPipeline(device, mComputeRootSignature.Get(), computeShader.get());
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 2);
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1 + 2);
	mGameInfoCB = std::make_unique<ConstantBuffer<GameInfoConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	mShadowMapRenderer->BuildDescriptorHeap(device, 3, 4);
	mComputePipeline->BuildDescriptorHeap(device);
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 3, 4);
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

	auto dustTex = make_shared<Texture>();
	dustTex->LoadTextureFromDDS(device, cmdList, L"Resources\\magicarray.dds");
	dustTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mPipelines[Layer::Particle]->AppendTexture(dustTex);

	// Normalmapped
	auto brickTex = make_shared<Texture>();
	brickTex->LoadTextureFromDDS(device, cmdList, L"Resources\\brick.dds");
	brickTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brickTex);

	auto brickNormal = make_shared<Texture>();
	brickNormal->LoadTextureFromDDS(device, cmdList, L"Resources\\brickNormalmap.dds");
	brickNormal->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brickNormal);

	auto brick2Tex = make_shared<Texture>();
	brick2Tex->LoadTextureFromDDS(device, cmdList, L"Resources\\brick2.dds");
	brick2Tex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brick2Tex);

	auto brick2Normal = make_shared<Texture>();
	brick2Normal->LoadTextureFromDDS(device, cmdList, L"Resources\\brick2normalmap.dds");
	brick2Normal->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::NormalMapped]->AppendTexture(brick2Normal);

	// Default
	auto carTex = make_shared<Texture>();
	carTex->LoadTextureFromDDS(device, cmdList, L"Resources\\CarTexture.dds");
	carTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Default]->AppendTexture(carTex);	

	auto tileTex = make_shared<Texture>();
	tileTex->LoadTextureFromDDS(device, cmdList, L"Resources\\tile.dds");
	tileTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Default]->AppendTexture(tileTex);	

	// Transparent
	auto iceTex = make_shared<Texture>();
	iceTex->LoadTextureFromDDS(device, cmdList, L"Resources\\ice.dds");
	iceTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Transparent]->AppendTexture(iceTex);

	auto copyBrick2Tex = make_shared<Texture>(*brick2Tex);
	mPipelines[Layer::Reflected]->AppendTexture(copyBrick2Tex);
	auto copyTileTex = make_shared<Texture>(*tileTex);
	mPipelines[Layer::Reflected]->AppendTexture(copyTileTex);
	auto copyCarTex = make_shared<Texture>(*carTex);
	mPipelines[Layer::Reflected]->AppendTexture(copyCarTex);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// terrain
	auto terrain = make_shared<TerrainObject>(1024, 1024, XMFLOAT3(1.0f, 1.0f, 1.0f));
	terrain->SetSRVIndex(0);
	terrain->BuildHeightMap(L"Resources\\heightmap.raw");
	terrain->BuildTerrainMesh(device, cmdList, 45, 45);
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
		float pos_x = Math::RandFloat(0, terrain->GetWidth()-1);
		float pos_z = Math::RandFloat(0, terrain->GetDepth()-1);
		float pos_y = terrain->GetHeight(pos_x, pos_z) + 4.0f;
		if (pos_y < 0.0f) pos_y = 0.0f;

		treeBillboard->AppendBillboard(XMFLOAT3(pos_x, pos_y, pos_z));
	}
	treeBillboard->BuildMesh(device, cmdList);
	mPipelines[Layer::Billboard]->AppendObject(treeBillboard);

	/*mFlameBillboard = make_shared<Billboard>(2.0f, 2.0f);
	mFlameBillboard->SetSRVIndex(2);
	mFlameBillboard->AppendBillboard(XMFLOAT3(0.0f,0.0f,0.0f));
	mFlameBillboard->BuildMesh(device, cmdList);*/

	auto particle = make_shared<ParticleMesh>(
		device, cmdList, mRoomCenter,
		XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),
		3000, 3.0f, 80);
	auto particleObj = make_shared<GameObject>();
	particleObj->SetSRVIndex(0);
	particleObj->SetMesh(particle);
	mPipelines[Layer::Particle]->AppendObject(particleObj);

	// boxes
	auto boxMesh = make_shared<BoxMesh>(device, cmdList, 4.0f, 4.0f, 4.0f);
	for (int i=0;i<100;i++)
	{
		float pos_x = Math::RandFloat(0, terrain->GetWidth()-1);
		float pos_z = Math::RandFloat(0, terrain->GetDepth()-1);
		float pos_y = terrain->GetHeight(pos_x, pos_z) + 10.0f;

		auto boxObj = make_shared<GameObject>();
		boxObj->SetMesh(boxMesh);
		boxObj->SetPosition(pos_x, pos_y, pos_z);
		boxObj->SetRotation(XMFLOAT3(0.0f, 1.0f, 0.0f), 100.0f);

		mPipelines[Layer::NormalMapped]->AppendObject(boxObj);
	}
	auto boxMesh2 = make_shared<BoxMesh>(device, cmdList, 2.0f, 2.0f, 2.0f);
	for (int i = 0; i < 10; i++)
	{
		float pos_x = Math::RandFloat(mRoomCenter.x - 40.0f, mRoomCenter.x + 40.0f);
		float pos_z = Math::RandFloat(mRoomCenter.z - 40.0f, mRoomCenter.z + 40.0f);
		auto box = make_shared<GameObject>();
		auto boxObj = make_shared<GameObject>();
		boxObj->SetMesh(boxMesh);
		boxObj->SetPosition(pos_x, 2.0f, pos_z);
		mPipelines[Layer::NormalMapped]->AppendObject(boxObj);
	}

	// car
	auto carMesh = make_shared<Mesh>();
	carMesh->LoadFromObj(device, cmdList, L"Models\\PoliceCar.obj");

	auto carObj = make_shared<TerrainPlayer>(terrain.get());
	carObj->SetMesh(carMesh);
	carObj->SetSRVIndex(0);
	carObj->SetPosition(mRoomCenter);	
	mPlayer = carObj.get();
	mPipelines[Layer::Default]->AppendObject(carObj);

	// plane : test
	auto shadowMapPlane = make_shared<GridMesh>(device, cmdList, 10.0f, 10.0f, 10.0f, 10.0f);
	auto planeObj = make_shared<GameObject>();
	planeObj->SetMesh(shadowMapPlane);
	planeObj->SetSRVIndex(0);
	planeObj->SetPosition({ mRoomCenter.x + 50.0f, mRoomCenter.y+7.0f, mRoomCenter.z +40.0f });
	planeObj->Rotate(0.0f, 90.0f, 0.0f);
	mPipelines[Layer::ShadowDebug]->AppendObject(planeObj);

	auto reflectedCarObj = make_shared<GameObject>();
	reflectedCarObj->SetReflected(XMFLOAT4(0.0f, 0.0f, 1.0f, -mRoomCenter.z -50.0f));
	reflectedCarObj->SetMesh(carMesh);
	reflectedCarObj->SetSRVIndex(2);
	mReflectedPlayer = reflectedCarObj.get();
	mPipelines[Layer::Reflected]->AppendObject(reflectedCarObj);

	float aspect = mMainCamera->GetAspect();
	mPlayerCamera.reset(mPlayer->ChangeCameraMode((int)CameraMode::THIRD_PERSON_CAMERA));
	mPlayerCamera->SetLens(0.25f * Math::PI, aspect, 1.0f, 5000.0f);
	mCurrentCamera = mPlayerCamera.get();
	
	BuildRoomObject(device, cmdList);
}

void GameScene::BuildRoomObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	const float uvMax = 10.0f;

	auto planeMesh1 = make_shared<GridMesh>(device, cmdList,   4.0f,  20.0f, uvMax, uvMax);
	auto planeMesh2 = make_shared<GridMesh>(device, cmdList, 108.0f,   4.0f, uvMax, uvMax);
	auto planeMesh3 = make_shared<GridMesh>(device, cmdList, 108.0f,  24.0f, uvMax, uvMax);
	auto planeMesh4 = make_shared<GridMesh>(device, cmdList, 108.0f, 108.0f, uvMax, uvMax);
	auto mirrorMesh = make_shared<GridMesh>(device, cmdList, 100.0f,  20.0f, 20.0f, 20.0f);

	// Mirror
	auto mirrorObj = make_shared<GameObject>();
	mirrorObj->SetSRVIndex(0);
	mirrorObj->SetMesh(mirrorMesh);
	mirrorObj->SetPosition(mRoomCenter.x, mRoomCenter.y + 10.0f, mRoomCenter.z +50.0f);
	mirrorObj->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines[Layer::Transparent]->AppendObject(mirrorObj);
	mPipelines[Layer::Mirror]->AppendObject(mirrorObj);

	// Front
	auto gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh1);
	gridObj->SetPosition(mRoomCenter.x - 52.0f, mRoomCenter.y + 10.0f, mRoomCenter.z +50.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);

	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh1);
	gridObj->SetPosition(mRoomCenter.x + 52.0f, mRoomCenter.y+ 10.0f, mRoomCenter.z +50.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);

	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh2);
	gridObj->SetPosition(mRoomCenter.x, mRoomCenter.y+22.0f, mRoomCenter.z +50.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);

	// Left
	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh3);
	gridObj->SetPosition(mRoomCenter.x - 54.0f, mRoomCenter.y+12.0f, mRoomCenter.z- 4.0f);
	gridObj->Rotate(0.0f, -90.0f, 0.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);
	auto copyGridObj = make_shared<GameObject>(*gridObj);
	copyGridObj->SetSRVIndex(0); 
	copyGridObj->SetReflected(XMFLOAT4(0.0f, 0.0f, 1.0f, -mRoomCenter.z -50.0f));
	mPipelines[Layer::Reflected]->AppendObject(copyGridObj);

	// Right
	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh3);
	gridObj->SetPosition(mRoomCenter.x + 54.0f, mRoomCenter.y+12.0f, mRoomCenter.z- 4.0f);
	gridObj->Rotate(0.0f, +90.0f, 0.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);
	copyGridObj = make_shared<GameObject>(*gridObj);
	copyGridObj->SetSRVIndex(0);
	copyGridObj->SetReflected(XMFLOAT4(0.0f, 0.0f, 1.0f, -mRoomCenter.z-50.0f));
	mPipelines[Layer::Reflected]->AppendObject(copyGridObj);

	// Back
	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(2);
	gridObj->SetMesh(planeMesh3);
	gridObj->SetPosition(mRoomCenter.x, mRoomCenter.y+12.0f, mRoomCenter.z- 58.0f);
	gridObj->Rotate(0.0f, 180.0f, 0.0f);
	mPipelines[Layer::NormalMapped]->AppendObject(gridObj);
	copyGridObj = make_shared<GameObject>(*gridObj);
	copyGridObj->SetSRVIndex(0);
	copyGridObj->SetReflected(XMFLOAT4(0.0f, 0.0f, 1.0f, -mRoomCenter.z-50.0f));
	mPipelines[Layer::Reflected]->AppendObject(copyGridObj);

	// Bottom
	gridObj = make_shared<GameObject>();
	gridObj->SetSRVIndex(1);
	gridObj->SetMesh(planeMesh4);
	gridObj->SetPosition(mRoomCenter.x, mRoomCenter.y, mRoomCenter.z - 4.0f);
	gridObj->Rotate(90.0f, 0.0f, 0.0f);
	mPipelines[Layer::Default]->AppendObject(gridObj);
	copyGridObj = make_shared<GameObject>(*gridObj);
	copyGridObj->SetSRVIndex(1);
	copyGridObj->SetReflected(XMFLOAT4(0.0f, 0.0f, 1.0f, -mRoomCenter.z-50.0f));
	mPipelines[Layer::Reflected]->AppendObject(copyGridObj);
}

void GameScene::PreRender(ID3D12GraphicsCommandList* cmdList)
{
	if (mShadowMapRenderer)
		mShadowMapRenderer->PreRender(cmdList, this);
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
		if (wParam == 'Q')
		{
			mLODSet = !mLODSet;
		}
		if (wParam == 'I')
		{
			if (mOutside) {
				mPlayer->SetPosition(mRoomCenter);
				mMainCamera->SetPosition(mRoomCenter);
				mPlayerCamera->SetPosition(mRoomCenter);
			}
			else
			{
				mPlayer->SetPosition(257, 100.0f, 257);
				mMainCamera->SetPosition(257, 120.0f, 257);
				mPlayerCamera->SetPosition(257, 120.0f, 257);
			}
			mOutside = !mOutside;
		}
		break;
	}
}

void GameScene::OnPreciseKeyInput(float elapsed)
{
	bool player_active = (mCurrentCamera == mPlayerCamera.get());

	if (GetAsyncKeyState('W') & 0x8000)
	{
		if (player_active) {
			mPlayer->Walk(500.0f * elapsed);
		}
		else
			mCurrentCamera->Walk(100.0f * elapsed);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		if (player_active) {
			mPlayer->Strafe(-500.0f * elapsed);
		}
		else
			mCurrentCamera->Strafe(-100.0f * elapsed);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		if (player_active) {
			mPlayer->Walk(-500.0f * elapsed);
		}
		else
			mCurrentCamera->Walk(-100.0f * elapsed);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		if (player_active) {
			mPlayer->Strafe(500.0f * elapsed);
		}
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

	UpdateLight(elapsed);
	mCurrentCamera->Update(elapsed);

	mShadowMapRenderer->UpdateDepthCamera(mMainLight);
	for (const auto& [_, pso] : mPipelines)
		pso->Update(elapsed, mCurrentCamera);

	mReflectedPlayer->SetWorld(mPlayer->GetWorld());
	
	//CreateAndAppendDustBillboard(device);
	//CollisionProcess(device);
	//DeleteTimeOverBillboards(device);

	UpdateConstants(timer);
}

void GameScene::UpdateLight(float elapsed)
{
	XMMATRIX R = XMMatrixRotationY(0.1f * elapsed);
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		// rotate each direction..
		mMainLight.Lights[i].Direction = Vector3::TransformNormal(mMainLight.Lights[i].Direction,R);
		mMainLight.Lights[i].Position = Vector3::Multiply(2.0f, mMainLight.Lights[i].Direction);
	}
}

void GameScene::UpdateLightConstants()
{
	mMainLight.ShadowTransform = Matrix4x4::Transpose(mShadowMapRenderer->GetShadowTransform(0));

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX reflect = XMMatrixReflect(mirrorPlane);
	LightConstants reflectedLightCnst = mMainLight;
	for (int i = 0; i < 3; i++) {
		XMVECTOR reflectedDirection = XMVector3TransformNormal(XMLoadFloat3(&reflectedLightCnst.Lights[i].Direction), reflect);
		XMStoreFloat3(&reflectedLightCnst.Lights[i].Direction, reflectedDirection);
	}

	mLightCB->CopyData(0, mMainLight);
	mLightCB->CopyData(1, reflectedLightCnst);
}

void GameScene::UpdateCameraConstant(int idx, Camera* camera)
{
	// 카메라로부터 상수를 받는다.
	mCameraCB->CopyData(idx, camera->GetConstants());
}

void GameScene::UpdateConstants(const GameTimer& timer)
{
	UpdateCameraConstant(0, mCurrentCamera);
	UpdateLightConstants();	

	GameInfoConstants gameInfo{};
	gameInfo.RandFloat4 = XMFLOAT4(
		Math::RandFloat(-1.0f, 1.0f),
		Math::RandFloat(0.0f, 1.0f),
		Math::RandFloat(-1.0f, 1.0f),
		Math::RandFloat(1.0f, 5.0f));
	gameInfo.PlayerPosition = mPlayer->GetPosition();
	gameInfo.KeyInput = mLODSet;
	gameInfo.CurrentTime = timer.CurrentTime();
	gameInfo.ElapsedTime = timer.ElapsedTime();

	mGameInfoCB->CopyData(0, gameInfo);
	
	//mShadowMapRenderer->UpdateDepthCamera(lightCnst);
	for (const auto& [_, pso] : mPipelines)
		pso->UpdateConstants();
}

void GameScene::SetCBV(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex)
{
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress(cameraCBIndex));
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress(0));
	cmdList->SetGraphicsRootConstantBufferView(2, mGameInfoCB->GetGPUVirtualAddress(0));
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
	RenderPipelines(cmdList, 0);

	mComputePipeline->SetCurrBackBuffer(cmdList, backBuffer);
	mComputePipeline->Dispatch(cmdList);
	mComputePipeline->CopyMapToRT(cmdList, backBuffer);
}

void GameScene::RenderPipelines(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex)
{	
	SetCBV(cmdList, cameraCBIndex);
	mShadowMapRenderer->SetShadowMapSRV(cmdList, 5);

	for (const auto& [layer, pso] : mPipelines) {
		if (layer == Layer::Reflected)
			cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress(1));

		pso->SetAndDraw(cmdList, (bool)mLODSet);

		if (layer == Layer::Reflected)
			cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress(0));		
	}
}

void GameScene::CollisionProcess(ID3D12Device* device)
{
	const vector<shared_ptr<GameObject>>& boxes = mPipelines[Layer::NormalMapped]->GetRenderObjects();
	
	bool flag = false;
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
				flag = true;
			}
		}
	}
	if(flag) mPipelines[Layer::NormalMapped]->ResetPipeline(device);
}

void GameScene::CreateAndAppendDustBillboard(ID3D12Device* device)
{	
	int randnum = Math::RandInt(100, 1500);
	auto randMillisec = std::chrono::milliseconds(randnum);

	auto currentTime = std::chrono::high_resolution_clock::now();
	if (currentTime - mPrevTime > randMillisec) {
		
		shared_ptr<Billboard> dust = make_shared<Billboard>(*mDustBillboard);
		XMFLOAT3 pos = mPlayer->GetPosition();

		float randX = Math::RandFloat(-1.0f, 1.0f);
		float randY = Math::RandFloat(0.0f, 1.0f);
		float randZ = Math::RandFloat(-1.0f, 1.0f);

		dust->SetPosition(pos);
		dust->SetMovement(XMFLOAT3( randX, randY, randZ ), 3.0f);
		dust->SetDurationTime(1500ms);
		mPipelines[Layer::Billboard]->AppendObject(dust);
		mPipelines[Layer::Billboard]->ResetPipeline(device);
		mPrevTime = std::chrono::high_resolution_clock::now();
	}
}

void GameScene::CreateAndAppendFlameBillboard(ID3D12Device* device, GameObject* box)
{
	static XMFLOAT3 FlameDirections[9] = {
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
	
	for (int i = 0; i < 9; i++)
	{
		shared_ptr<Billboard> newFlame = make_shared<Billboard>(*mFlameBillboard);
		newFlame->SetPosition(box->GetPosition());
		newFlame->SetMovement(FlameDirections[i], 1.0f);
		newFlame->SetDurationTime(2500ms);
		mPipelines[Layer::Billboard]->AppendObject(newFlame);
	}
	mPipelines[Layer::Billboard]->ResetPipeline(device);
}

void GameScene::DeleteTimeOverBillboards(ID3D12Device* device)
{
	bool flag = false;
	auto current_time = std::chrono::steady_clock::now();
	auto& allBillboards = mPipelines[Layer::Billboard]->GetRenderObjects();
	for (int i = (int)allBillboards.size()-1; i >= 0; i--)
	{
		if (dynamic_cast<Billboard*>(allBillboards[i].get())->IsTimeOver(current_time))
		{
			flag = true;
			mPipelines[Layer::Billboard]->DeleteObject(i);
		}
	}
	if (flag) mPipelines[Layer::Billboard]->ResetPipeline(device);
}


