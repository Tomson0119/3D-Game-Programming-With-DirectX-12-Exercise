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

void GameScene::Update(const GameTimer& timer)
{
	OnPreciseKeyInput(timer);

	float offset = (float)mMaxBoardSize / (mMaxBoardSize - 1);
	float playerPosX = -4.0f + (float)(mPlayerPosCol * offset);
	float playerPosZ = -4.0f + (float)(mPlayerPosRow * offset);
	mPlayer->SetPosition(playerPosX, 0.0f, playerPosZ);

	for (const auto& [_, pso] : mPipelines)
		pso->Update(timer.ElapsedTime());
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress());
	
	mPipelines["texLit"]->SetAndDraw(cmdList);
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::OnPreciseKeyInput(const GameTimer& timer)
{
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && !mKeyStates[VK_RIGHT])
	{
		if(mPlayerPosCol < mMaxBoardSize - 1)
			mPlayerPosCol += 1;
		mKeyStates[VK_RIGHT] = true;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000 && !mKeyStates[VK_LEFT])
	{
		if (mPlayerPosCol > 0)
			mPlayerPosCol -= 1;
		mKeyStates[VK_LEFT] = true;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000 && !mKeyStates[VK_UP])
	{
		if (mPlayerPosRow < mMaxBoardSize - 1)
			mPlayerPosRow += 1;
		mKeyStates[VK_UP] = true;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000 && !mKeyStates[VK_DOWN])
	{
		if (mPlayerPosRow > 0)
			mPlayerPosRow -= 1;
		mKeyStates[VK_DOWN] = true;
	}

	for (auto& [key, state] : mKeyStates)
	{
		if(!GetAsyncKeyState(key) && state) 
			state = false;
	}
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descRanges[2];
	descRanges[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	descRanges[1] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	

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

	mPipelines["texLit"] = make_unique<Pipeline>();
	mPipelines["texLit"]->BuildPipeline(device, mRootSignature.Get(), texShader.get());
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 2, 3);
}

void GameScene::BuildTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto boardTex = make_shared<Texture>();
	boardTex->CreateTextureResource(device, cmdList, L"Resources\\board.dds");
	boardTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["texLit"]->AppendTexture(boardTex);

	auto sideTex = make_shared<Texture>();
	sideTex->CreateTextureResource(device, cmdList, L"Resources\\brown.dds");
	sideTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["texLit"]->AppendTexture(sideTex);

	auto whiteTex = make_shared<Texture>();
	whiteTex->CreateTextureResource(device, cmdList, L"Resources\\white.dds");
	whiteTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["texLit"]->AppendTexture(whiteTex);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto gridMesh1 = make_shared<GridMesh>(device, cmdList, 10.0f, 10.0f);
	auto gridMesh2 = make_shared<GridMesh>(device, cmdList, 10.0f, 0.6f);
	auto pawnMesh = make_shared<Mesh>();
	pawnMesh->LoadFromBinary(device, cmdList, L"Models\\pawn.bin");
	
	auto top = make_shared<GameObject>();
	top->SetMesh(gridMesh1);
	top->SetSRVIndex(0);
	top->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines["texLit"]->AppendObject(top);

	auto bottom = make_shared<GameObject>();
	bottom->SetMesh(gridMesh1);
	bottom->SetSRVIndex(1);
	bottom->Rotate(180.0f, 0.0f, 0.0f);
	bottom->Upward(-0.6f, false);
	bottom->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines["texLit"]->AppendObject(bottom);

	for (int i = 0; i < 4; i++)
	{
		auto side = make_shared<GameObject>();
		side->SetMesh(gridMesh2);
		side->SetSRVIndex(1);

		if (i == 0)
			side->Rotate(-90.0f, 0.0f, 0.0f);
		else if (i == 1)
			side->Rotate(90.0f, 0.0f, 0.0f);
		else if (i == 2)
			side->Rotate(90.0f, 0.0f, 90.0f);
		else
			side->Rotate(90.0f, 0.0f, -90.0f);

		side->Upward(5.0f);
		side->Upward(-0.3f, false);
		side->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
		mPipelines["texLit"]->AppendObject(side);
	}

	auto pawn = make_shared<GameObject>();
	pawn->SetMesh(pawnMesh);
	pawn->SetSRVIndex(2);
	pawn->SetPosition(-4.0f, 0.0f, -4.0f);
	pawn->Scale(0.5f, 0.5f, 0.5f);
	pawn->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);
	mPipelines["texLit"]->AppendObject(pawn);

	mPlayer = pawn.get();
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);
}
