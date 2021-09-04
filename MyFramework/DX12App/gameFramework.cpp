#include "common.h"
#include "gameFramework.h"
#include "gameScene.h"
#include "camera.h"
#include "player.h"
#include "frameResource.h"

using namespace std;

DXGI_SAMPLE_DESC gMsaaStateDesc;

GameFramework::GameFramework()
	: D3DFramework()
{
	// 전역 변수 멀티샘플링 서술자 세팅
	// 파이프라인 객체를 생성할 때 쓰인다.
	gMsaaStateDesc.Count = (mMsaa4xEnable) ? 4 : 1;
	gMsaaStateDesc.Quality = (mMsaa4xEnable) ? (mMsaa4xQualityLevels - 1) : 0;

	mScenes.push(std::make_unique<GameScene>());
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

	mCamera = std::make_unique<Camera>();
	
	for (int i = 0; i < mFrameCount; i++)
		mFrameResources[i] = make_unique<FrameResource>(

			);
	if (!mScenes.empty())
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
	if (mCamera) mCamera->SetLens(GetAspect());
}

void GameFramework::OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (!mScenes.empty())
			mScenes.top()->OnProcessMouseDown(m_hwnd, wParam);
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (!mScenes.empty())
			mScenes.top()->OnProcessMouseUp(wParam);
		break;

	case WM_MOUSEMOVE:
		if (!mScenes.empty())
			mScenes.top()->OnProcessMouseMove(wParam);
		break;
	}
}

void GameFramework::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			if (mScenes.size() > 1) {
				mScenes.pop();
				mScenes.top()->BuildObjects(mD3dDevice.Get(), mCommandList.Get());
				// TODO: 새로운 씬을 빌드하고나면 커맨드 큐에 명령들을 옮겨야 하기
				// 때문에 테스트가 필요하다.
				break;
			}
			PostQuitMessage(0);
			return;

		case VK_F9:
			D3DFramework::ChangeFullScreenState();
			break;
		}
		break;
	}
	if (!mScenes.empty()) mScenes.top()->OnProcessKeyInput(uMsg, wParam, lParam);
}

void GameFramework::Update(const GameTimer& timer)
{
	D3DFramework::UpdateFrameStates();

	if (!mScenes.empty()) mScenes.top()->Update(timer);
}

void GameFramework::Draw(const GameTimer& timer)
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

	if (!mScenes.empty()) mScenes.top()->Draw(mCommandList.Get(), timer);

	// 화면 버퍼의 상태를 다시 PRESENT 상태로 전이한다.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	ThrowIfFailed(mSwapChain->Present(0, 0));  // 화면버퍼를 Swap한다.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex(); // 현재 후면버퍼의 인덱스를 갱신한다.

	// WaitForGPUComplete();
	
	// 울타리 지점을 전진시키고 현재 프레임자원의 울타리에 갱신한다.
	mCurrFrameResource->mFence = ++mCurrentFenceValue;

	// 커맨드 큐에 울타리 지점을 설정하는 명령을 추가한다.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFenceValue));
}
