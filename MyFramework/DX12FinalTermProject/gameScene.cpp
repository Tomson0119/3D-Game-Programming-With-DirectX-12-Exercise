#include "stdafx.h"
#include "gameScene.h"


GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>();
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);
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

	mCamera->UpdateViewMatrix();

	for (const auto& obj : mGameObjects)
		obj->Update(dt);

	UpdateConstants();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress());
	
	if (!mShowWireFrame)
		mPipelines["defaultColor"]->SetAndDraw(cmdList, mObjectCB.get());
	else
	{
		mPipelines["wiredColor"]->SetAndDraw(cmdList, mObjectCB.get());
		//mPipelines["boundingBox"]->SetAndDraw(cmdList, mObjectCB.get());
	}
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState)
{
	SetCapture(hwnd);
	GetCursorPos(&mLastMousePos);
}

void GameScene::OnProcessMouseUp(WPARAM buttonState)
{
	ReleaseCapture();
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == 'G')
			mShowWireFrame = !mShowWireFrame;
		break;
	}
}

void GameScene::OnKeyboardInput(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Walk(10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Strafe(-10.0f * dt);
	
	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Walk(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Strafe(10.0f * dt);

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		mCamera->Upward(10.0f * dt);

	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		mCamera->Upward(-10.0f * dt);
}

void GameScene::OnMouseInput(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetCapture() != nullptr)
	{
		POINT currMousePos;
		GetCursorPos(&currMousePos);
		
		float delta_x = XMConvertToRadians(0.25f * static_cast<float>(currMousePos.x - mLastMousePos.x));
		float delta_y = XMConvertToRadians(0.25f * static_cast<float>(currMousePos.y - mLastMousePos.y));

		mCamera->RotateY(delta_x);
		mCamera->Pitch(delta_y);

		mLastMousePos = currMousePos;
	}
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
	/*mMeshes["body"] = std::make_unique<Mesh>();
	mMeshes["body"]->LoadFromBinary(device, cmdList, L"Models\\Gun.bin");
	mMeshes["slide"] = std::make_unique<Mesh>();
	mMeshes["slide"]->LoadFromBinary(device, cmdList, L"Models\\Slide.bin");

	auto pistol = std::make_unique<GameObject>(0, mMeshes["body"].get());
	pistol->SetPosition(0.0f, 0.0f, 0.0f);
	pistol->SetMaterial(XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.125f);

	auto slide = std::make_unique<GameObject>(1, mMeshes["slide"].get());
	slide->SetPosition(0.0f, 2.0f, 0.0f);
	slide->SetMaterial(XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.125f);

	mPipelines["defaultColor"]->SetObject(pistol.get());
	mPipelines["defaultColor"]->SetObject(slide.get());
	
	mPipelines["wiredColor"]->SetObject(pistol.get());
	mPipelines["wiredColor"]->SetObject(slide.get());

	mGameObjects.emplace_back(std::move(pistol));
	mGameObjects.emplace_back(std::move(slide));*/

	mMeshes["grid"] = std::make_unique<GridMesh>(device, cmdList, 10, 10, XMFLOAT3(1.0f, 1.0f, 1.0f));
	mMeshes["box"] = std::make_unique<BoxMesh>(device, cmdList, 2.0f, 2.0f, 2.0f);

	auto grid = std::make_unique<GameObject>(0, mMeshes["grid"].get());
	grid->SetPosition(0.0f, 0.0f, 0.0f);
	grid->SetMaterial(XMFLOAT4(0.0f, 0.2f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.125f);

	mPipelines["defaultColor"]->SetObject(grid.get());
	mPipelines["wiredColor"]->SetObject(grid.get());

	mGameObjects.emplace_back(std::move(grid));

	auto box = std::make_unique<GameObject>(1, mMeshes["box"].get());
	box->SetPosition(0.0f, 0.0f, 0.0f);
	box->SetMaterial(XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f), XMFLOAT3(0.4f, 0.4f, 0.4f), 0.5f);

	mPipelines["defaultColor"]->SetObject(box.get());
	mPipelines["wiredColor"]->SetObject(box.get());
	mGameObjects.emplace_back(std::move(box));

}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjects.size());
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	mShaders["defaultColor"] = std::make_unique<ColorShader>(L"Shaders\\defaultColor.hlsl");
	mShaders["onlyColor"] = std::make_unique<ColorShader>(L"Shaders\\onlyColor.hlsl");

	mPipelines["defaultColor"] = std::make_unique<Pipeline>();
	mPipelines["defaultColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultColor"].get());

	mPipelines["wiredColor"] = std::make_unique<Pipeline>(true);
	mPipelines["wiredColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["onlyColor"].get());
}