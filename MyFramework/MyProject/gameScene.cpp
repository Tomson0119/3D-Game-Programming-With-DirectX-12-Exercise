#include "../MyCommon/stdafx.h"
#include "gameScene.h"

GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>();
	mCamera->SetPosition(0.0f, 0.0f, -10.0f);

	mSun.Diffuse = XMFLOAT3(1.0f, 1.0f, 1.0f);
	mSun.Direction = Vector3::Normalize(XMFLOAT3(1.0f, 1.0f, -1.0f));
	mSun.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);  // 태양광은 위치정보가 의미 없다.
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
	cameraCnst.CameraPos = mCamera->GetPosition();
	mCameraCB->CopyData(0, cameraCnst);

	LightConstants lightCnst;
	lightCnst.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lightCnst.Lights[0] = mSun;
	mLightCB->CopyData(0, lightCnst);

	for (const auto& obj : mGameObjects)
	{
		obj->Update();  // 오브젝트의 상태를 업데이트한다.
		
		// 오브젝트로부터 상수들을 받아 업데이트한다.
		mObjectCB->CopyData(obj->CBIndex(), obj->GetObjectConstants());
	}
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
	
	auto cmCB = mCameraCB->Resource();	
	cmdList->SetGraphicsRootConstantBufferView(0, cmCB->GetGPUVirtualAddress());

	auto litCB = mLightCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, litCB->GetGPUVirtualAddress());
	
	for (const auto& [name, pso] : mPipelines)
	{
		cmdList->SetPipelineState(pso->GetPSO());
		pso->Draw(cmdList, mObjectCB->Resource()->GetGPUVirtualAddress(), mObjectCB->GetByteSize());
	}
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(hwnd);
}

void GameScene::OnProcessMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

void GameScene::OnProcessMouseMove(WPARAM buttonState, int x, int y)
{
	if (buttonState & VK_LBUTTON)
	{
		float delta_x = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float delta_y = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Rotate camera local axis
		mCamera->RotateY(delta_x);
		mCamera->Pitch(delta_y);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER parameters[3];	
	parameters[0].InitAsConstantBufferView(0);  // CommonCB
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
	mMeshes["car"] = std::make_unique<CarMesh>();
	mMeshes["car"]->LoadFromBinary(device, cmdList, L"Models\\racing_car.bin");

	auto carObject = std::make_unique<ColorObject>(0, mMeshes["car"].get());
	carObject->SetMaterial(XMFLOAT4(0.2f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);

	mPipelines["defaultColor"]->SetObject(carObject.get());
	mGameObjects.push_back(std::move(carObject));
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjects.size());
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	// Shader
	mShaders["color"] = std::make_unique<ColorShader>(L"Shaders\\color.hlsl");
	
	// Pipeline
	mPipelines["defaultColor"] = std::make_unique<Pipeline>();
	mPipelines["defaultColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["color"].get());
}
