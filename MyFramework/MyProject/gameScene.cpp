#include "../MyCommon/stdafx.h"
#include "gameScene.h"

GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>();
	mCamera->LookAt(
		XMFLOAT3(0.0f, 5.0f, -10.0f),
		XMFLOAT3(0.0f, 0.0f, 3.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f));
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
	ProcessInputKeyboard(timer);

	// 카메라의 상태를 업데이트한다.
	mCamera->UpdateViewMatrix();

	// 카메라로부터 상수를 받는다.
	CameraConstants cameraCnst;
	cameraCnst.View = Matrix4x4::Transpose(mCamera->GetView());
	cameraCnst.Proj = Matrix4x4::Transpose(mCamera->GetProj());
	cameraCnst.ViewProj = Matrix4x4::Transpose(Matrix4x4::Multiply(mCamera->GetView(), mCamera->GetProj()));
	cameraCnst.CameraPos = mCamera->GetPosition();
	mCameraCB->CopyData(0, cameraCnst);

	// 광원과 관련된 상수버퍼를 초기화 및 업데이트한다.
	LightConstants lightCnst;
	lightCnst.Ambient = XMFLOAT4(0.7f, 0.7f, 0.7f, 0.7f);

	lightCnst.Lights[0].Diffuse = { 0.8f, 0.8f, 0.8f };
	lightCnst.Lights[0].Direction = { -1.0f, 1.0f, -1.0f };

	lightCnst.Lights[1].Diffuse = { 0.15f, 0.15f, 0.15f };
	lightCnst.Lights[1].Direction = { 0.0f, 1.0f, 1.0f };

	lightCnst.Lights[2].Diffuse = { 0.35f, 0.35f, 0.35f };
	lightCnst.Lights[2].Direction = { 1.0f, 1.0f, -1.0f };

	mLightCB->CopyData(0, lightCnst);

	for (const auto& obj : mGameObjects)
		// 오브젝트로부터 상수들을 받아 업데이트한다.
		obj->Update(mObjectCB.get());

	for (const auto& bb : mBoundBoxes)
		// 게임 오브젝트의 개수만큼 Offset을 지정한다.
		bb->Update(mObjectCB.get());  
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
		mPipelines["boundingBox"]->SetAndDraw(cmdList, mObjectCB.get());
	}
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState)
{
	SetCapture(hwnd);
	GetCursorPos(&mLastMousePos);
	SetCursor(NULL);
}

void GameScene::OnProcessMouseUp(WPARAM buttonState)
{
	//ReleaseCapture();
}

void GameScene::OnProcessMouseMove(WPARAM buttonState)
{
	if (GetCapture() != nullptr) {
		POINT currMousePos;
		GetCursorPos(&currMousePos);
		SetCursorPos(mLastMousePos.x, mLastMousePos.y);

		float delta_x = XMConvertToRadians(0.25f * static_cast<float>(currMousePos.x - mLastMousePos.x));
		float delta_y = XMConvertToRadians(0.25f * static_cast<float>(currMousePos.y - mLastMousePos.y));

		// Rotate camera local axis
		mCamera->RotateY(delta_x);
		mCamera->Pitch(delta_y);
	}
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if(wParam == 0x47)
			mShowWireFrame = !mShowWireFrame;
		break;
	}
}

void GameScene::ProcessInputKeyboard(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Move(0.0f, 0.0f, 10.0f * dt);
	
	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Move(0.0f, 0.0f, -10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Move(-10.0f * dt, 0.0f, 0.0f);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Move(10.0f * dt, 0.0f, 0.0f);

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		mCamera->Move(XMFLOAT3(0.0f, 1.0f, 0.0f), 10.0f * dt);

	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		mCamera->Move(XMFLOAT3(0.0f, 1.0f, 0.0f), -10.0f * dt);

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		mPlayer->MoveForward(5.0f * dt);
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
	mMeshes["racing_car"] = std::make_unique<CarMesh>(device, cmdList, L"Models\\racing_car.bin");
	mMeshes["police_car"] = std::make_unique<CarMesh>(device, cmdList, L"Models\\police_car.bin");
	mMeshes["box1"] = std::make_unique<BoxMesh>(device, cmdList, 2.6f, 0.2f, 400.0f);
	mMeshes["box2"] = std::make_unique<BoxMesh>(device, cmdList, 0.2f, 0.2f, 400.0f);

	UINT index = 0;

	// 플레이어 객체 생성
	auto racing_car = std::make_unique<Player>(index++, mMeshes["racing_car"].get());
	racing_car->SetPosition(0.0f, 0.0f, 0.0f);
	racing_car->SetMaterial(XMFLOAT4(0.6f, 0.4f, 0.4f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.125f);
	racing_car->EnableBoundBoxRender(index++, device, cmdList);  // 바운딩 박스를 렌더링하도록 오브젝트를 생성한다.

	mPlayer = racing_car.get();
	mPipelines["defaultColor"]->SetObject(racing_car.get());
	mPipelines["wiredColor"]->SetObject(racing_car.get());
	mPipelines["boundingBox"]->SetObject(racing_car->GetBoundBoxObject());
	mBoundBoxes.push_back(racing_car->GetBoundBoxObject());
	mGameObjects.push_back(std::move(racing_car));

	// 도로를 구분하는 구분선
	for (int i = 0; i < 6; ++i)
	{
		auto line = std::make_unique<GameObject>(index++, mMeshes["box2"].get());
		line->SetMaterial(XMFLOAT4(1.0f, 0.5f, 0.1f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.4f);
		line->SetPosition(-7.0f + 2.8f * i, -0.1f, 190.0f);
		mPipelines["defaultColor"]->SetObject(line.get());
		mGameObjects.push_back(std::move(line));
	}

	// 아스팔트 도로
	for (int i = 0; i < 5; ++i) {
		auto road = std::make_unique<GameObject>(index++, mMeshes["box1"].get());
		road->SetPosition(-5.6f + 2.8f * i, -0.1f, 190.0f);
		road->SetMaterial(XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.2f);
		mPipelines["defaultColor"]->SetObject(road.get());
		mPipelines["wiredColor"]->SetObject(road.get());
		mGameObjects.push_back(std::move(road));
	}

	for (int i = 0; i < 2; ++i)
	{
		auto wall = std::make_unique<GameObject>(index++, mMeshes["box1"].get());
		wall->SetMaterial(XMFLOAT4(1.0f, 0.8f, 0.5f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.5f);
		wall->SetPosition(-7.2f + 14.4f * i, 1.1f, 190.0f);
		wall->EnableBoundBoxRender(index++, device, cmdList);
		wall->Rotate(0.0f, 0.0f, 90.0f);
		mWallObjects[i] = wall.get();
		mPipelines["defaultColor"]->SetObject(wall.get());
		mPipelines["wiredColor"]->SetObject(wall.get());
		mPipelines["boundingBox"]->SetObject(wall->GetBoundBoxObject());
		mBoundBoxes.push_back(wall->GetBoundBoxObject());
		mGameObjects.push_back(std::move(wall));
	}	
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
	mObjectCB = std::make_unique<ConstantBuffer<ObjectConstants>>(device, mGameObjects.size() + mBoundBoxes.size());
	mCameraCB = std::make_unique<ConstantBuffer<CameraConstants>>(device, 1);
	mLightCB = std::make_unique<ConstantBuffer<LightConstants>>(device, 1);
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
	// Shader
	mShaders["defaultColor"] = std::make_unique<ColorShader>(L"Shaders\\defaultColor.hlsl");
	mShaders["onlyColor"] = std::make_unique<ColorShader>(L"Shaders\\onlyColor.hlsl");

	// Pipeline
	mPipelines["defaultColor"] = std::make_unique<Pipeline>();
	mPipelines["defaultColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultColor"].get());

	// Wired Frame Pipeline
	mPipelines["wiredColor"] = std::make_unique<Pipeline>(true);
	mPipelines["wiredColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultColor"].get());

	// BoundingBox Pipeline
	mPipelines["boundingBox"] = std::make_unique<Pipeline>(true);
	mPipelines["boundingBox"]->BuildPipeline(device, mRootSignature.Get(), mShaders["onlyColor"].get());
}
