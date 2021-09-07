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
	// 카메라로부터 상수를 받는다.
	mCameraCB->CopyData(0, camera->GetConstants());

	// 광원과 관련된 상수버퍼를 초기화 및 업데이트한다.
	LightConstants lightCnst;
	lightCnst.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f);

	lightCnst.Lights[0].Diffuse = { 0.8f, 0.8f, 0.8f };
	lightCnst.Lights[0].Direction = { -1.0f, 1.0f, -1.0f };

	lightCnst.Lights[1].Diffuse = { 0.15f, 0.15f, 0.15f };
	lightCnst.Lights[1].Direction = { 0.0f, 1.0f, 1.0f };

	lightCnst.Lights[2].Diffuse = { 0.35f, 0.35f, 0.35f };
	lightCnst.Lights[2].Direction = { 1.0f, 1.0f, -1.0f };

	mLightCB->CopyData(0, lightCnst);

	//for (const auto& obj : mGameObjects)
	//	// 오브젝트로부터 상수들을 받아 업데이트한다.
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
	
	mPipelines["defaultLit"]->SetAndDraw(cmdList);
	mPipelines["wiredLit"]->SetAndDraw(cmdList);
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
	//D3D12_DESCRIPTOR_RANGE descRange = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	//D3D12_ROOT_PARAMETER parameters[4];
	//parameters[0] = Extension::DescriptorTable(1, &descRange, D3D12_SHADER_VISIBILITY_PIXEL);			   // Texture
	//parameters[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);  // CameraCB
	//parameters[2] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_ALL);  // LightCB
	//parameters[3] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 2, D3D12_SHADER_VISIBILITY_ALL);  // ObjectCB

	//D3D12_STATIC_SAMPLER_DESC samplerDesc = Extension::SamplerDesc(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	//D3D12_ROOT_SIGNATURE_DESC rootSigDesc = Extension::RootSignatureDesc(_countof(parameters), parameters, 
	//	1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_ROOT_PARAMETER parameters[3];
	parameters[0] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);  // CameraCB
	parameters[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_ALL);  // LightCB
	parameters[2] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 2, D3D12_SHADER_VISIBILITY_ALL);  // ObjectCB

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
		IID_PPV_ARGS(&mRootSignature)));
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	auto shader = make_unique<DefaultShader>(L"Shaders\\defaultLit.hlsl");

	mPipelines["defaultLit"] = make_unique<Pipeline>();
	mPipelines["defaultLit"]->BuildPipeline(device, mRootSignature.Get(), shader.get());

	mPipelines["wiredLit"] = make_unique<Pipeline>();
	mPipelines["wiredLit"]->SetWiredFrame(true);
	mPipelines["wiredLit"]->BuildPipeline(device, mRootSignature.Get(), shader.get());
}

void GameScene::BuildDescriptorHeap(ID3D12Device* device)
{
	for (const auto& [_, pso] : mPipelines)
		pso->BuildDescriptorHeap(device);
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	const int BoxCount = 500;

	shared_ptr<BoxMesh> boxMesh = make_shared<BoxMesh>(device, cmdList, 1.0f, 1.0f, 1.0f);
	
	//// Creating texture.
	//auto boxTex = make_shared<Texture>();
	//boxTex->CreateTextureResource(device, cmdList, L"Resources\\box.dds");

	for (int i = 0; i < BoxCount; i++) {
		shared_ptr<GameObject> box = make_shared<GameObject>();
		box->SetMesh(boxMesh);
		box->SetPosition(-BoxCount + 1.1f * i, 0.0f, 0.0f);
		box->SetMaterial(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);

		mPipelines["defaultLit"]->AppendObject(box);

		shared_ptr<GameObject> wired = make_shared<GameObject>();
		wired->SetMesh(boxMesh);
		wired->SetPosition(-BoxCount + 1.1f * i, 0.0f, -1.5f);
		wired->SetMaterial(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);

		mPipelines["wiredLit"]->AppendObject(wired);

		mGameObjectCount += 2;
	}
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	//mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjectCount);
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);

	for (const auto& [_, pso] : mPipelines)
		pso->BuildConstantBuffer(device, 2);
}
