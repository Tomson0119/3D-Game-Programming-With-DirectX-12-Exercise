#include "../MyCommon/stdafx.h"
#include "gameScene.h"

GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>(0.25f * Math::PI, 1.0f, 1.0f, 1000.0f);
}

GameScene::~GameScene()
{
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	BuildRootSignature(device);
	BuildShaders();
	BuildConstantBuffers(device);
	BuildPSOs(device);
}

void GameScene::Update(const GameTimer& timer)
{
	mCamera->UpdateViewMatrix();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	auto common = mCommonCB->Resource();	
	cmdList->SetGraphicsRootConstantBufferView(0, common->GetGPUVirtualAddress());
	
	for (const auto& pso : mPipelines)
	{
		pso.second->SetPipeline(cmdList);
		pso.second->Draw(cmdList);
	}
}

void GameScene::OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER parameters[2];	
	parameters[0].InitAsConstantBufferView(0);  // CommonCB
	parameters[1].InitAsConstantBufferView(1);  // ObjectCB

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, parameters, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void GameScene::BuildGameObjects()
{
}

void GameScene::BuildShaders()
{
	auto colorShader = std::make_unique<ColorShader>();
	mShaders["color"] = std::move(colorShader);
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mCommonCB = std::make_unique<ConstantBuffer<CommonConstants>>(device, 1);
}

void GameScene::BuildPSOs(ID3D12Device* device)
{
	auto colorPSO = std::make_unique<Pipeline>();
	colorPSO->BuildPipeline(device, mRootSignature.Get(), mShaders["color"].get());
	mPipelines["defaultColor"] = std::move(colorPSO);
}
