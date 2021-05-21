#include "../MyCommon/stdafx.h"
#include "gameScene.h"

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	BuildRootSignature(device);
	BuildPSOs(device);

}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	// cmdList->SetGraphicsRootConstantBufferView(0, virtualAdress)
	
	for (const auto& pso : mPipelines)
	{
		pso.second->SetPipeline(cmdList);
		pso.second->Draw(cmdList);
	}
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER parameters[1];	
	parameters[0].InitAsConstantBufferView(0);

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

void GameScene::BuildShaders()
{
	auto colorShader = std::make_unique<ColorShader>();
	mShaders["color"] = std::move(colorShader);
}

void GameScene::BuildPSOs(ID3D12Device* device)
{
	auto colorPSO = std::make_unique<Pipeline>();
	colorPSO->BuildPipeline(device, mRootSignature.Get(), mShaders["color"].get());
	mPipelines["defaultColor"] = std::move(colorPSO);
}
