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

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
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
	auto defaultShader = make_unique<DefaultShader>(L"Shaders\\defaultLit.hlsl");
	auto dynamicCubeShader = make_unique<DefaultShader>(L"Shaders\\cubemap.hlsl");

	mPipelines[Layer::SkyBox] = make_unique<SkyboxPipeline>(device, cmdList);
	mPipelines[Layer::SkyBox]->BuildPipeline(device, mRootSignature.Get());

	mPipelines[Layer::Terrain] = make_unique<Pipeline>();
	mPipelines[Layer::Terrain]->BuildPipeline(device, mRootSignature.Get(), terrainShader.get());

	mPipelines[Layer::Billboard] = make_unique<Pipeline>();
	mPipelines[Layer::Billboard]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	mPipelines[Layer::Billboard]->BuildPipeline(device, mRootSignature.Get(), billboardShader.get());

	mPipelines[Layer::Default] = make_unique<Pipeline>();
	mPipelines[Layer::Default]->BuildPipeline(device, mRootSignature.Get(), defaultShader.get());

	mCubeMapRenderer = make_unique<DynamicCubeRenderer>();
	mCubeMapRenderer->BuildPipeline(device, mRootSignature.Get(), dynamicCubeShader.get());
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1+6);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);

	mCubeMapRenderer->BuildConstantBuffer(device);
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 2, 3);

	mCubeMapRenderer->BuildDescriptorHeap(device, 2, 3);
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

	// Default
	auto brickTex = make_shared<Texture>();
	brickTex->LoadTextureFromDDS(device, cmdList, L"Resources\\brick.dds");
	brickTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Default]->AppendTexture(brickTex);

	auto brickNormal = make_shared<Texture>();
	brickNormal->LoadTextureFromDDS(device, cmdList, L"Resources\\brickNormalmap.dds");
	brickNormal->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines[Layer::Default]->AppendTexture(brickNormal);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto terrain = make_shared<TerrainObject>(1024, 1024);
	terrain->SetSRVIndex(0);
	terrain->BuildHeightMap(L"Resources\\heightmap.raw");
	terrain->BuildTerrainMesh(device, cmdList);
	mPipelines[Layer::Terrain]->AppendObject(terrain);

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

	auto boxMesh = make_shared<BoxMesh>(device, cmdList, 10.0f, 10.0f, 10.0f);
	auto boxObj = make_shared<GameObject>();
	boxObj->SetMesh(boxMesh);
	boxObj->SetSRVIndex(0);
	boxObj->SetPosition(512.0f, 400.0f, 512.0f);
	mPipelines[Layer::Default]->AppendObject(boxObj);

	auto sphereMesh = std::make_shared<SphereMesh>(device, cmdList, 10.0f, 20, 20);
	auto cube = std::make_shared<DynamicCubeMapObject>(device, cmdList, 256);
	cube->SetMesh(sphereMesh);
	cube->SetPosition(512, 500.0f, 512);
	cube->SetMaterial(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.98f, 0.97f, 0.95f), 0.1f);	
	mCubeMapRenderer->AppendObject(device, cube);
}

void GameScene::PrepareCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mCubeMapRenderer->PreDraw(device, cmdList, this);
}

void GameScene::Update(const GameTimer& timer, Camera* camera)
{
	OnPreciseKeyInput(timer);

	for (const auto& [_, pso] : mPipelines)
		pso->Update(timer.ElapsedTime(), camera);

	mCubeMapRenderer->Update(timer.ElapsedTime(), camera);
}

void GameScene::UpdateCameraConstant(int idx, Camera* camera)
{
	// 카메라로부터 상수를 받는다.
	mCameraCB->CopyData(idx, camera->GetConstants());
}

void GameScene::UpdateConstants(Camera* camera)
{
	UpdateCameraConstant(0, camera);

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

	mCubeMapRenderer->UpdateConstants();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, int cameraCBIndex, bool predraw)
{	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress(cameraCBIndex));
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress(0));
	
	for (const auto& [layer, pso] : mPipelines)
		if(layer != Layer::Billboard) pso->SetAndDraw(cmdList);

	if(!predraw)
		mCubeMapRenderer->SetAndDraw(cmdList);
}


