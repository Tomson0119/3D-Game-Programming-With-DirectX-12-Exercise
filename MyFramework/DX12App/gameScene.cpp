#include "stdafx.h"
#include "gameScene.h"

using namespace std;

GameScene::GameScene()
{
	//mCamera = std::make_unique<Camera>();
	//mCamera->SetPosition(0.0f, 0.0f, -10.0f);
}

GameScene::~GameScene()
{
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	BuildRootSignature(device);
	BuildShadersAndPSOs(device);
	BuildGameObjects(device, cmdList);
	BuildConstantBuffers(device);
	BuildDescriptorHeap(device);
}

void GameScene::UpdateConstants(Camera* camera)
{
	// ī�޶�κ��� ����� �޴´�.
	mCameraCB->CopyData(0, camera->GetConstants());

	// ������ ���õ� ������۸� �ʱ�ȭ �� ������Ʈ�Ѵ�.
	LightConstants lightCnst;
	lightCnst.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

	lightCnst.Lights[0].Diffuse = { 0.5f, 0.5f, 0.5f };
	lightCnst.Lights[0].Direction = { -1.0f, 1.0f, -1.0f };

	lightCnst.Lights[1].Diffuse = { 0.15f, 0.15f, 0.15f };
	lightCnst.Lights[1].Direction = { 0.0f, 1.0f, 1.0f };

	lightCnst.Lights[2].Diffuse = { 0.35f, 0.35f, 0.35f };
	lightCnst.Lights[2].Direction = { 1.0f, 1.0f, -1.0f };

	mLightCB->CopyData(0, lightCnst);

	//for (const auto& obj : mGameObjects)
	//	// ������Ʈ�κ��� ������� �޾� ������Ʈ�Ѵ�.
	//	obj->UpdateConstants(mObjectCB.get());
	for (const auto& [_, pso] : mPipelines)
		pso->UpdateConstants();
}

void GameScene::Update(const GameTimer& timer)
{
	OnPreciseKeyInput(timer);

	/*for (const auto& obj : mGameObjects)
		obj->Update(timer.ElapsedTime(), nullptr);*/
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
	switch (uMsg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case 0x51:
			mShowWired = !mShowWired;
			break;
		}
		break;
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
	auto shader = make_unique<DefaultShader>(L"Shaders\\texShader.hlsl");

	mPipelines["texLit"] = make_unique<Pipeline>();
	mPipelines["texLit"]->BuildPipeline(device, mRootSignature.Get(), shader.get());
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device, 2, 3);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	const int BoxCount = 500;

	shared_ptr<BoxMesh> boxMesh = make_shared<BoxMesh>(device, cmdList, 1.0f, 1.0f, 1.0f);
	
	// Creating texture.
	auto boxTex = make_shared<Texture>();
	boxTex->CreateTextureResource(device, cmdList, L"Resources\\box.dds");
	boxTex->SetDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
	mPipelines["texLit"]->AppendTexture(boxTex);

	for (int i = 0; i < BoxCount; i++) {
		shared_ptr<GameObject> box = make_shared<GameObject>();
		box->SetMesh(boxMesh);
		box->SetSRVIndex(0);
		box->SetPosition(-BoxCount + 1.1f * i, 0.0f, 0.0f);
		box->SetMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.25f);

		mPipelines["texLit"]->AppendObject(box);
	}
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	//mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjectCount);
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device);
}
