#include "stdafx.h"
#include "gameFramework.h"
#include "camera.h"

using namespace std;

GameFramework::GameFramework()
	: D3DFramework()
{
	mScenes.emplace(std::make_unique<GameScene>());
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

	// �ʱ�ȭ�ϴ� ��ɾ �ֱ� ���� Ŀ�ǵ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	//mCamera = make_unique<Camera>();
	//mCamera->SetLens(0.25f * Math::PI, 1.0f, 1.0f, 1000.0f);

	if (!mScenes.empty())
		mScenes.top()->BuildObjects(mD3dDevice.Get(), mCommandList.Get());

	// Command List�� �ݰ� Queue�� ��ɾ �ƴ´�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// �ʱ�ȭ�� ����� ������ ��ٸ���.
	WaitUntilGPUComplete();

	return true;
}

void GameFramework::OnResize()
{
	D3DFramework::OnResize();
	//if(mCamera) mCamera->SetLens(GetAspect());
	if (!mScenes.empty()) mScenes.top()->Resize(GetAspect());
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

void GameFramework::Update(const GameTimer& timer)
{
	D3DFramework::UpdateFrameStates();

	//mCamera->Update(timer.ElapsedTime());
	if (!mScenes.empty()) mScenes.top()->Update(timer);
}

void GameFramework::Draw(const GameTimer& timer)
{
	// ��ɾ� �Ҵ��ڸ� ���� �ʱ�ȭ���ش�.
	ThrowIfFailed(mCommandAllocator->Reset());

	// Command List�� Pipeline State�� ���´�. 
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// ȭ�� ������ ���¸� Render Target ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// ȭ�� ���ۿ� ���� ���ٽ� ���۸� �ʱ�ȭ�Ѵ�.
	XMFLOAT4 color = (!mScenes.empty()) ? mScenes.top()->GetFrameColor() : (XMFLOAT4)Colors::White;

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (FLOAT*)&color, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// �������� ���۸� ��ü������ �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), TRUE, &DepthStencilView());

	if (!mScenes.empty()) mScenes.top()->Draw(mCommandList.Get());

	// ȭ�� ������ ���¸� �ٽ� PRESENT ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// Ŀ�ǵ� ����Ʈ�� ��ɾ���� �� �����ϱ���� ��ٸ���.
	WaitUntilGPUComplete();

	ThrowIfFailed(mSwapChain->Present(0, 0));  // ȭ����۸� Swap�Ѵ�.

	// ���� �ĸ���� ��ġ�� �̵��� �� �ٽ� ��ٸ���.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	WaitUntilGPUComplete();
}