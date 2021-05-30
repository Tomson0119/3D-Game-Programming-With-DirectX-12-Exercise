#include "../MyCommon/stdafx.h"
#include "gameScene.h"

GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>();
	mCamera->SetPosition(0.0f, 0.0f, -10.0f);
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
}

void GameScene::Resize(float aspect)
{
	mCamera->SetLens(0.25f * Math::PI, aspect, 1.0f, 1000.0f);
}

void GameScene::Update(const GameTimer& timer)
{
	// 카메라의 상태를 업데이트한다.
	mCamera->UpdateViewMatrix();

	// 카메라로부터 상수를 받는다.
	CameraConstants cameraCnst;
	cameraCnst.View = Matrix4x4::Transpose(mCamera->GetView());
	cameraCnst.Proj = Matrix4x4::Transpose(mCamera->GetProj());
	cameraCnst.ViewProj = Matrix4x4::Transpose(Matrix4x4::Multiply(mCamera->GetView(), mCamera->GetProj()));
	mCameraCB->CopyData(0, cameraCnst);

	ObjectConstants objCnst;
	for (UINT i = 0; i < mGameObjects.size(); ++i)
	{
		mGameObjects[i]->Update();  // 오브젝트의 상태를 업데이트한다.
		
		// 오브젝트로부터 상수를 받아 업데이트한다.
		objCnst.World = Matrix4x4::Transpose(mGameObjects[i]->GetWorld());
		mObjectCB->CopyData(i, objCnst);
	}
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
	
	auto cmCB = mCameraCB->Resource();	
	cmdList->SetGraphicsRootConstantBufferView(0, cmCB->GetGPUVirtualAddress());
	
	for (const auto& [name, pso] : mPipelines)
	{
		cmdList->SetPipelineState(pso->GetPSO());
		pso->Draw(cmdList, mObjectCB->Resource()->GetGPUVirtualAddress(), mObjectCB->GetByteSize());
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

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(parameters), parameters, 
		0, nullptr,	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mMeshes["box"] = std::make_unique<BoxMesh>(device, cmdList, 4.0f, 4.0f, 4.0f, true);

	auto boxObject = std::make_unique<GameObject>(0, mMeshes["box"].get());
	mPipelines["defaultColor"]->SetObject(boxObject.get());
	mGameObjects.push_back(std::move(boxObject));
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjects.size());
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	// Shader
	mShaders["color"] = std::make_unique<ColorShader>(L"Shaders\\color.hlsl");

	// Pipeline
	mPipelines["defaultColor"] = std::make_unique<Pipeline>();
	mPipelines["defaultColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["color"].get());
}
