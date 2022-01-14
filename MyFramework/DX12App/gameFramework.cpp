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

	// �ʱ�ȭ�ϴ� ��ɾ �ֱ� ���� Ŀ�ǵ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mScenes.push(make_unique<GameScene>());
	mScenes.top()->BuildObjects(mD3dDevice.Get(), mCommandList.Get(), GetAspect());

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
	if (!mScenes.empty()) mScenes.top()->OnResize(GetAspect());
}

void GameFramework::OnProcessMouseDown(WPARAM buttonState, int x, int y)
{
	mScenes.top()->OnProcessMouseDown(m_hwnd, buttonState, x, y);
}

void GameFramework::OnProcessMouseUp(WPARAM buttonState, int x, int y)
{
	mScenes.top()->OnProcessMouseUp(buttonState, x, y);
}

void GameFramework::OnProcessMouseMove(WPARAM buttonState, int x, int y)
{
	mScenes.top()->OnProcessMouseMove(buttonState, x, y);
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
	mScenes.top()->OnProcessKeyInput(uMsg, wParam, lParam);
}

void GameFramework::OnPreciseKeyInput()
{
}

void GameFramework::Update()
{
	D3DFramework::UpdateFrameStates();
	
	OnPreciseKeyInput();

	//mCamera->Update(mTimer.ElapsedTime());
	mScenes.top()->Update(mD3dDevice.Get(), mTimer);
}

void GameFramework::Draw()
{
	// ��ɾ� �Ҵ��ڸ� ���� �ʱ�ȭ���ش�.
	ThrowIfFailed(mCommandAllocator->Reset());

	// Command List�� Pipeline State�� ���´�. 
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCommandList->SetGraphicsRootSignature(mScenes.top()->GetRootSignature());

	mScenes.top()->PreRender(mCommandList.Get());

	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// ȭ�� ������ ���¸� Render Target ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ȭ�� ���ۿ� ���� ���ٽ� ���۸� �ʱ�ȭ�Ѵ�.
	XMFLOAT4 color = mScenes.top()->GetFrameColor();

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (FLOAT*)&color, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// �������� ���۸� ��ü������ �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), TRUE, &DepthStencilView());

	mScenes.top()->Draw(mCommandList.Get(), CurrentBackBuffer());

	// ȭ�� ������ ���¸� �ٽ� PRESENT ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// Ŀ�ǵ� ����Ʈ�� ��ɾ���� �� �����ϱ���� ��ٸ���.
	WaitUntilGPUComplete();

	ThrowIfFailed(mD3dDevice->GetDeviceRemovedReason());
	ThrowIfFailed(mSwapChain->Present(0, 0));  // ȭ����۸� Swap�Ѵ�.	

	// ���� �ĸ���� ��ġ�� �̵��� �� �ٽ� ��ٸ���.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	WaitUntilGPUComplete();
}

