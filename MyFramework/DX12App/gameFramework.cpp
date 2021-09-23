#include "stdafx.h"
#include "gameFramework.h"
#include "camera.h"

using namespace std;

GameFramework::GameFramework()
	: D3DFramework()
{
}

GameFramework::~GameFramework()
{
	size_t size = mScenes.size();
	for (int i = 0; i < size; ++i) mScenes.pop();
}

bool GameFramework::InitFramework()
{
	if (!D3DFramework::InitFramework())
		return false;

	// 초기화하는 명령어를 넣기 위해 커맨드 리스트를 개방한다.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCamera = make_unique<Camera>();
	mCamera->SetPosition(0.0f, 0.0f, 0.0f);
	mCamera->SetLens(0.25f * Math::PI, GetAspect(), 1.0f, 1000.0f);
	mCamera->Pitch(30.0f);
	mCamera->Walk(-mCameraRadius);

	mScenes.push(make_unique<GameScene>());
	mScenes.top()->BuildObjects(mD3dDevice.Get(), mCommandList.Get());

	// Command List를 닫고 Queue에 명령어를 싣는다.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// 초기화가 진행될 때까지 기다린다.
	WaitUntilGPUComplete();

	return true;
}

void GameFramework::OnResize()
{
	D3DFramework::OnResize();
	if(mCamera) mCamera->SetLens(GetAspect());
}

void GameFramework::OnProcessMouseDown(WPARAM buttonState, int x, int y)
{
	if (buttonState & MK_LBUTTON)
	{
		SetCapture(m_hwnd);
		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
	if (!mScenes.empty()) mScenes.top()->OnProcessMouseDown(buttonState, x, y);
}

void GameFramework::OnProcessMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
	if (!mScenes.empty()) mScenes.top()->OnProcessMouseUp(buttonState, x, y);
}

void GameFramework::OnProcessMouseMove(WPARAM buttonState, int x, int y)
{
	if ((buttonState & MK_LBUTTON) && GetCapture())
	{
		float dx = static_cast<float>(x - mLastMousePos.x);

		mLastMousePos.x = x;
		mLastMousePos.y = y;

		mCamera->Walk(mCameraRadius);
		mCamera->RotateY(0.25f * dx);
		mCamera->Walk(-mCameraRadius);
	}
	if (!mScenes.empty()) mScenes.top()->OnProcessMouseMove(buttonState);
}

void GameFramework::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			if (mScenes.size() > 0)
				mScenes.pop();

			if (mScenes.empty()) {
				PostQuitMessage(0);
				return;
			}
			break;

		case VK_F9:
			D3DFramework::ChangeFullScreenState();
			break;
		}
		break;
	}
	if (!mScenes.empty()) mScenes.top()->OnProcessKeyInput(uMsg, wParam, lParam);
}

void GameFramework::OnPreciseKeyInput()
{
	const float elapsed = mTimer.ElapsedTime();

	if (GetAsyncKeyState('W') & 0x8000) {
		//mCamera->Walk(5.0f * elapsed);
	}

	if (GetAsyncKeyState('A') & 0x8000) {
		//mCamera->Strafe(-5.0f * elapsed);
	}

	if (GetAsyncKeyState('S') & 0x8000) {
		//mCamera->Walk(-5.0f * elapsed);
	}

	if (GetAsyncKeyState('D') & 0x8000) {
		//mCamera->Strafe(5.0f * elapsed);
	}
}

void GameFramework::Update()
{
	D3DFramework::UpdateFrameStates();
	
	OnPreciseKeyInput();

	mCamera->Update(mTimer.ElapsedTime());
	if (!mScenes.empty()) {
		mScenes.top()->Update(mTimer);
		mScenes.top()->UpdateConstants(mCamera.get());
	}
}

void GameFramework::Draw()
{
	// 명령어 할당자를 먼저 초기화해준다.
	ThrowIfFailed(mCommandAllocator->Reset());

	// Command List를 Pipeline State로 묶는다. 
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 화면 버퍼의 상태를 Render Target 상태로 전이한다.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// 화면 버퍼와 깊이 스텐실 버퍼를 초기화한다.
	XMFLOAT4 color = (!mScenes.empty()) ? mScenes.top()->GetFrameColor() : (XMFLOAT4)Colors::White;

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (FLOAT*)&color, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// 렌더링할 버퍼를 구체적으로 설정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), TRUE, &DepthStencilView());

	if (!mScenes.empty()) mScenes.top()->Draw(mCommandList.Get());

	// 화면 버퍼의 상태를 다시 PRESENT 상태로 전이한다.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// 커맨드 리스트의 명령어들을 다 실행하기까지 기다린다.
	WaitUntilGPUComplete();

	ThrowIfFailed(mSwapChain->Present(0, 0));  // 화면버퍼를 Swap한다.

	// 다음 후면버퍼 위치로 이동한 후 다시 기다린다.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	WaitUntilGPUComplete();
}
