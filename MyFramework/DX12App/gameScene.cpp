#include "stdafx.h"
#include "gameScene.h"

using namespace std;

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
	mAspect = aspect;
	mCamera->SetLens(0.25f * Math::PI, aspect, 1.0f, 500.0f);
}

void GameScene::UpdateConstants()
{
	// 카메라로부터 상수를 받는다.
	CameraConstants cameraCnst;
	cameraCnst.View = Matrix4x4::Transpose(mCamera->GetView());
	cameraCnst.Proj = Matrix4x4::Transpose(mCamera->GetProj());
	cameraCnst.ViewProj = Matrix4x4::Transpose(Matrix4x4::Multiply(mCamera->GetView(), mCamera->GetProj()));
	cameraCnst.CameraPos = mCamera->GetPosition();
	cameraCnst.Aspect = mAspect;
	mCameraCB->CopyData(0, cameraCnst);

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

	for (const auto& obj : mGameObjects)
		// 오브젝트로부터 상수들을 받아 업데이트한다.
		obj->UpdateConstants(mObjectCB.get());	
}

void GameScene::Update(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	OnKeyboardInput(timer);
	OnMouseInput(timer);

	mCamera->Update(dt);

	for (const auto& obj : mGameObjects)
		obj->Update(dt, nullptr);

	UpdateConstants();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress());
	
	mPipelines["defaultLit"]->SetAndDraw(cmdList, mObjectCB.get());
}

void GameScene::OnKeyboardInput(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetAsyncKeyState('W') & 0x8000) {
		mCamera->Walk(5.0f * dt);
	}

	if (GetAsyncKeyState('A') & 0x8000) {
		mCamera->Strafe(-5.0f * dt);
	}
	
	if (GetAsyncKeyState('S') & 0x8000) {
		mCamera->Walk(-5.0f * dt);
	}

	if (GetAsyncKeyState('D') & 0x8000) {
		mCamera->Strafe(5.0f * dt);
	}
}

void GameScene::OnMouseInput(const GameTimer& timer)
{
	
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER parameters[3];	
	parameters[0].InitAsConstantBufferView(0);  // CameraCB
	parameters[1].InitAsConstantBufferView(1);  // LightCB
	parameters[2].InitAsConstantBufferView(2);  // ObjectCB

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
	mMeshes["box"] = make_unique<BoxMesh>(device, cmdList, 1.0f, 1.0f, 1.0f);
	
	auto box = make_unique<GameObject>(0, mMeshes["box"].get());
	box->SetPosition(0.0f, 0.0f, 0.0f);
	box->SetMaterial(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);

	mPipelines["defaultLit"]->SetObject(box.get());
	mGameObjects.push_back(move(box));
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, (UINT)mGameObjects.size());
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	mShaders["defaultLit"] = std::make_unique<DefaultShader>(L"Shaders\\defaultLit.hlsl");

	mPipelines["defaultLit"] = std::make_unique<Pipeline>(false);
	mPipelines["defaultLit"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultLit"].get());
}