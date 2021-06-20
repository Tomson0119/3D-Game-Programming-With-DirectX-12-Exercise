#include "stdafx.h"
#include "gameScene.h"


////////////////////////////////////////////////////////////////////////////
//
std::ostream& operator<<(std::ostream& os, XMFLOAT3& vec)
{
	os << vec.x << " " << vec.y << " " << vec.z;
	return os;
}


GameScene::GameScene()
{
	mCamera = std::make_unique<Camera>();
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

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());	
	cmdList->SetGraphicsRootConstantBufferView(0, mCameraCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mLightCB->GetGPUVirtualAddress());
	
	if (!mShowWireFrame)
	{
		mPipelines["defaultLit"]->SetAndDraw(cmdList, mObjectCB.get());
		mPipelines["defaultColor"]->SetAndDraw(cmdList, mObjectCB.get());
	}
	else
	{
		mPipelines["wiredLit"]->SetAndDraw(cmdList, mObjectCB.get());
		mPipelines["wiredColor"]->SetAndDraw(cmdList, mObjectCB.get());
	}
	
	if (mCamera->GetMode() == CameraMode::FIRST_PERSON_CAMERA) {
		mPipelines["line"]->SetAndDraw(cmdList, mObjectCB.get());
		mPipelines["screenDiffuse"]->SetAndDraw(cmdList, mObjectCB.get());
	}
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState)
{
	if (!GetCapture()) {
		SetCapture(hwnd);
		GetCursorPos(&mLastMousePos);
		ShowCursor(FALSE);
	}
	else if(mCamera->GetMode() == CameraMode::FIRST_PERSON_CAMERA) {
		XMFLOAT3 pos = mPlayer->GetPosition();
		
		XMFLOAT3 begin = dynamic_cast<GunPlayer*>(mPlayer)->GetMuzzlePos();
		begin.y += 0.5f;  // 값을 살짝 조정.
		XMFLOAT3 screenPos = CenterPointScreenToWorld();
		screenPos.z = mPlayer->GetPosition().z;
		XMFLOAT3 collapsed = GetCollisionPosWithObjects(screenPos, mPlayer->GetLook());
		std::cout << collapsed << std::endl;

		XMFLOAT3 dir = Vector3::Normalize(Vector3::Subtract(collapsed, begin));
		XMFLOAT3 newPos = Vector3::Add(begin, dir, 0.5f * mBulletTrack->GetLength());
		
		mBulletTrack->SetPosition(newPos);
		mBulletTrack->SetLook(dir);

		Test->SetPosition(collapsed);
	}
}

void GameScene::OnProcessMouseUp(WPARAM buttonState)
{
	//ReleaseCapture();
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'G':
			mShowWireFrame = !mShowWireFrame;
			break;

		case '1':
			if (mPlayer) {
				auto newCamera = mPlayer->ChangeCameraMode((int)CameraMode::FIRST_PERSON_CAMERA);
				if (newCamera) mCamera.reset(newCamera);
			}
			break;

		case '2':
			if (mPlayer) {
				auto newCamera = mPlayer->ChangeCameraMode((int)CameraMode::THIRD_PERSON_CAMERA);
				if (newCamera) mCamera.reset(newCamera);
			}
			break;

		case '3':
			if (mPlayer) {
				auto newCamera = mPlayer->ChangeCameraMode((int)CameraMode::TOP_DOWN_CAMERA);
				if (newCamera) mCamera.reset(newCamera);
			}
			break;

		case VK_CONTROL:
			ReleaseCapture();
			ShowCursor(TRUE);
			break;
		}
		Resize(mAspect);
		break;
	}
}

void GameScene::OnKeyboardInput(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetAsyncKeyState('W') & 0x8000) {
		mPlayer->Walk(5.0f);
	}

	if (GetAsyncKeyState('A') & 0x8000) {
		mPlayer->Strafe(-5.0f);
	}
	
	if (GetAsyncKeyState('S') & 0x8000) {
		mPlayer->Walk(-5.0f);
	}

	if (GetAsyncKeyState('D') & 0x8000) {
		mPlayer->Strafe(5.0f);
	}
}

void GameScene::OnMouseInput(const GameTimer& timer)
{
	const float dt = timer.ElapsedTime();

	if (GetCapture() != nullptr)
	{
		POINT currMousePos;
		GetCursorPos(&currMousePos);
		SetCursorPos(mLastMousePos.x, mLastMousePos.y);
		
		float delta_x = 0.25f * static_cast<float>(currMousePos.x - mLastMousePos.x);
		float delta_y = 0.25f * static_cast<float>(currMousePos.y - mLastMousePos.y);

		mPlayer->RotateY(delta_x);
		mPlayer->Pitch(delta_y);
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
	// Terrain
	auto terrain = std::make_unique<TerrainObject>(0);
	terrain->BuildTerrainMeshes(device, cmdList, 257, 257,
		XMFLOAT3(2.0f, 0.5f, 2.0f), XMFLOAT4(0.2f, 0.4f, 0.0f, 1.0f),
		L"Resources\\heightmap1.raw");
	mTerrain = terrain.get();

	mPipelines["defaultColor"]->SetObject(terrain.get());
	mPipelines["wiredColor"]->SetObject(terrain.get());

	// Player(Box + Gun)
	mMeshes["box"] = std::make_unique<BoxMesh>(device, cmdList, 1.0f, 3.0f, 1.0f);	
	mMeshes["gun_body"] = std::make_unique<Mesh>(device, cmdList, L"Models\\GunBody.bin");
	mMeshes["gun_slide"] = std::make_unique<Mesh>(device, cmdList, L"Models\\GunSlide.bin");

	mMeshes["testBox"] = std::make_unique<BoxMesh>(device, cmdList, 0.2f, 0.2f, 0.2f);
	
	mMeshes["slime"] = std::make_unique<Mesh>(device, cmdList, L"Models\\Slime.bin");

	auto gunSlide = std::make_unique<GunObject>(1, mMeshes["gun_slide"].get());
	gunSlide->SetPosition(1.0f, 3.0f, 3.0f);
	gunSlide->SetMaterial(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.4f);
	gunSlide->Rotate(0.0f, -90.0f, 0.0f);
	
	auto gunBody = std::make_unique<GunObject>(2, mMeshes["gun_body"].get());
	gunBody->SetPosition(1.0f, 3.0f, 3.0f);
	gunBody->SetMaterial(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.4f);
	gunBody->Rotate(0.0f, -90.0f, 0.0f);

	auto box = std::make_unique<GunPlayer>(3, mMeshes["box"].get(), terrain.get());
	box->SetMaterial(XMFLOAT4(0.8f, 0.6f, 0.0f, 1.0f), XMFLOAT3(0.4f, 0.4f, 0.4f), 0.125f);
	box->SetChild(gunBody.get());
	box->SetChild(gunSlide.get());

	mPlayer = box.get();
	mCamera.reset(mPlayer->ChangeCameraMode((int)CameraMode::FIRST_PERSON_CAMERA));
	Resize(mAspect);  // Resetting Lens

	mPipelines["defaultLit"]->SetObject(gunSlide.get());
	mPipelines["defaultLit"]->SetObject(gunBody.get());
	mPipelines["defaultLit"]->SetObject(box.get());

	mPipelines["wiredLit"]->SetObject(gunSlide.get());
	mPipelines["wiredLit"]->SetObject(gunBody.get());
	mPipelines["wiredLit"]->SetObject(box.get());

	auto line = std::make_unique<LineObject>(4, device, cmdList, 60.0f);
	line->SetPosition(-100.0f, -100.0f, -100.0f);
	mBulletTrack = line.get();
	mPipelines["line"]->SetObject(line.get());

	auto crossHair = std::make_unique<CrossHairObject>(5, device, cmdList);
	mPipelines["screenDiffuse"]->SetObject(crossHair.get());

	auto test = std::make_unique<GameObject>(6, mMeshes["testBox"].get());
	test->SetMaterial(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.1f);
	test->SetPosition(-100.0f, -100.0f, -100.0f);
	Test = test.get();
	mPipelines["defaultLit"]->SetObject(test.get());

	const std::array<XMFLOAT4, 4> colors =
	{
		XMFLOAT4(0.6f, 0.4f, 0.2f, 1.0f),
		XMFLOAT4(0.3f, 0.4f, 1.0f, 1.0f),
		XMFLOAT4(1.0f, 0.6f, 1.0f, 1.0f),
		XMFLOAT4(0.4f, 0.0f, 0.7f, 1.0f)
	};

	for (int i = 0; i < 10; ++i)
	{
		auto enemy = std::make_unique<EnemyObject>(i + 7, mMeshes["slime"].get(), terrain.get(), mPlayer);
		enemy->SetMaterial(colors[i%4], XMFLOAT3(0.0f, 0.0f, 0.0f), 0.3f);
		enemy->SetPosition(255.0f + i * 2.0f, 60.0f, 300.0f);
		mEnemies[i] = enemy.get();
		mPipelines["defaultLit"]->SetObject(enemy.get());
		mPipelines["wiredLit"]->SetObject(enemy.get());
		mGameObjects.emplace_back(std::move(enemy));
	}

	mGameObjects.emplace_back(std::move(terrain));
	mGameObjects.emplace_back(std::move(gunSlide));
	mGameObjects.emplace_back(std::move(gunBody));
	mGameObjects.emplace_back(std::move(box));
	mGameObjects.emplace_back(std::move(line));
	mGameObjects.emplace_back(std::move(crossHair));
	mGameObjects.emplace_back(std::move(test));
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
	mShaders["defaultColor"] = std::make_unique<DefaultShader>(L"Shaders\\defaultColor.hlsl");
	mShaders["diffuse"] = std::make_unique<DiffuseShader>(L"Shaders\\diffuse.hlsl");
	mShaders["screen"] = std::make_unique<DiffuseShader>(L"Shaders\\screenDiffuse.hlsl");

	mPipelines["defaultLit"] = std::make_unique<Pipeline>(false);
	mPipelines["defaultLit"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultLit"].get());

	mPipelines["wiredLit"] = std::make_unique<Pipeline>(true);
	mPipelines["wiredLit"]->BuildPipeline(device, mRootSignature.Get(), mShaders["defaultColor"].get());

	mPipelines["defaultColor"] = std::make_unique<Pipeline>(false);
	mPipelines["defaultColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["diffuse"].get());

	mPipelines["wiredColor"] = std::make_unique<Pipeline>(true);
	mPipelines["wiredColor"]->BuildPipeline(device, mRootSignature.Get(), mShaders["diffuse"].get());

	mPipelines["screenDiffuse"] = std::make_unique<Pipeline>(false);
	mPipelines["screenDiffuse"]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	mPipelines["screenDiffuse"]->BuildPipeline(device, mRootSignature.Get(), mShaders["screen"].get());

	mPipelines["line"] = std::make_unique<Pipeline>(false);
	mPipelines["line"]->SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	mPipelines["line"]->BuildPipeline(device, mRootSignature.Get(), mShaders["diffuse"].get());
}

XMFLOAT3 GameScene::CenterPointScreenToWorld()
{
	XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };

	XMMATRIX view = XMLoadFloat4x4(&mCamera->GetView());
	const XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX proj = XMLoadFloat4x4(&mCamera->GetProj());
	const XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

	center = Vector3::Transform(center, invProj);
	center = Vector3::Transform(center, invView);

	return center;
}

XMFLOAT3 GameScene::GetCollisionPosWithObjects(XMFLOAT3& start, XMFLOAT3& dir)
{
	const float maxRange = 60.0f;
	float currentRange = 0.0f;
	XMFLOAT3 point = start;
	while(true)
	{
		point = Vector3::Add(point, dir, 0.01f);
		currentRange += 0.01f;

		if (point.y <= mTerrain->GetHeight(point.x, point.z)
			|| currentRange >= maxRange)
			return point;		
		
		if (OnCollisionWithEnemy(point))
			return point;
	}
}

bool GameScene::OnCollisionWithEnemy(XMFLOAT3& point)
{
	for (const auto& enemy : mEnemies)
	{
		XMFLOAT3 center = enemy->GetBoundingBox().Center;
		XMFLOAT3 extent = enemy->GetBoundingBox().Extents;

		float bound[6] = { center.x - extent.x, center.x + extent.x,
						   center.y - extent.y, center.y + extent.y,
						   center.z - extent.z, center.z + extent.z };

		if (bound[0] <= point.x && point.x <= bound[1]
			&& bound[2] <= point.y && point.y <= bound[3]
			&& bound[4] <= point.z && point.z <= bound[5])
		{
			// Do collision process
			enemy->GotShot();
			return true;
		}
	}
	return false;
}
