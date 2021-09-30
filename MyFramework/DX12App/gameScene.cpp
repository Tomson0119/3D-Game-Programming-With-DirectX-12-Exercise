#include "stdafx.h"
#include "gameScene.h"

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
	BuildShadersAndPSOs(device);
	BuildTextures(device, cmdList);
	BuildGameObjects(device, cmdList);
	BuildConstantBuffers(device);
	BuildDescriptorHeap(device);
}

void GameScene::UpdateConstants(Camera* camera)
{
	// 카메라로부터 상수를 받는다.
	mCameraCB->CopyData(0, camera->GetConstants());

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
}

void GameScene::Update(const GameTimer& timer, Camera* camera)
{
	OnPreciseKeyInput(timer);

	for (const auto& [_, pso] : mPipelines)
		pso->Update(timer.ElapsedTime());

	for (const auto& obj : mBillboards)
		obj->UpdateLook(camera);
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress());
	
	for (const auto& [_, pso] : mPipelines)
		pso->SetAndDraw(cmdList);
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::OnPreciseKeyInput(const GameTimer& timer)
{
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descRanges[2];
	descRanges[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	descRanges[1] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	

	D3D12_ROOT_PARAMETER parameters[4];
	parameters[0] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);  // CameraCB
	parameters[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_ALL);  // LightCB
	parameters[2] = Extension::DescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_ALL);			   // Object,  CBV
	parameters[3] = Extension::DescriptorTable(1, &descRanges[1], D3D12_SHADER_VISIBILITY_PIXEL);		   // Texture, SRV

	D3D12_STATIC_SAMPLER_DESC samplerDesc = Extension::SamplerDesc(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = Extension::RootSignatureDesc(_countof(parameters), parameters, 
		1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	auto texShader = make_unique<DefaultShader>(L"Shaders\\texShader.hlsl");
	auto diffTexShader = make_unique<DiffuseTexShader>(L"Shaders\\diffuseTex.hlsl");
	auto billboardShader = make_unique<DefaultShader>(L"Shaders\\billboard.hlsl");

	mPipelines["texLit"] = make_unique<Pipeline>();
	mPipelines["texLit"]->BuildPipeline(device, mRootSignature.Get(), texShader.get());

	mPipelines["diffTex"] = make_unique<Pipeline>();
	mPipelines["diffTex"]->BuildPipeline(device, mRootSignature.Get(), diffTexShader.get());

	mPipelines["billboard"] = make_unique<Pipeline>();
	mPipelines["billboard"]->BuildPipeline(device, mRootSignature.Get(), billboardShader.get());
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 2, 3);
}

void GameScene::BuildTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto boardTex = make_shared<Texture>();
	boardTex->CreateTextureResource(device, cmdList, L"Resources\\box.dds");
	boardTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["texLit"]->AppendTexture(boardTex);

	auto grassTex = make_shared<Texture>();
	grassTex->CreateTextureResource(device, cmdList, L"Resources\\grass.dds");
	grassTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["diffTex"]->AppendTexture(grassTex);

	auto gravelTex = make_shared<Texture>();
	gravelTex->CreateTextureResource(device, cmdList, L"Resources\\gravel.dds");
	gravelTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["diffTex"]->AppendTexture(gravelTex);

	auto treeArrayTex = make_shared<Texture>();
	treeArrayTex->CreateTextureResource(device, cmdList, L"Resources\\treearray.dds");
	treeArrayTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	mPipelines["billboard"]->AppendTexture(treeArrayTex);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto boxMesh = make_shared<BoxMesh>(device, cmdList, 5.0f, 5.0f, 5.0f);
	auto box = make_shared<GameObject>();
	box->SetMesh(boxMesh);
	box->SetSRVIndex(0);
	box->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines["texLit"]->AppendObject(box);

	auto terrain = make_shared<TerrainObject>(1025, 1025);
	terrain->Scale(1.0f, 0.4f, 1.0f);
	terrain->BuildHeightMap(L"Resources\\island.raw");
	terrain->BuildTerrainMesh(device, cmdList, XMFLOAT4(0.1f, 0.3f, 0.0f, 1.0f));
	terrain->SetSRVIndex(0);
	terrain->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines["diffTex"]->AppendObject(terrain);

	for (int i = 0; i < 100; i++) {
		auto billboard = make_shared<Billboard>(device, cmdList, 5.0f, 5.0f);

		float hw = (float)terrain->GetWidth() * 0.5f;
		float hd = (float)terrain->GetDepth() * 0.5f;
		
		float pos_x = Math::RandFloat(-hw, hw);
		float pos_z = Math::RandFloat(-hd, hd);

		float pos_y = terrain->GetHeight(pos_x, pos_z);
		if (pos_y < 0.0f) pos_y = 0.0f;
		billboard->SetPosition(pos_x, pos_y + 2.5f, pos_z);

		billboard->SetSRVIndex(0);
		billboard->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
		mPipelines["billboard"]->AppendObject(billboard);
		mBillboards.push_back(billboard.get());
	}
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);
}
