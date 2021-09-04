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
	// ���� ���� ��Ƽ���ø� ������ ����
	// ���������� ��ü�� ������ �� ���δ�.
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

	// �ʱ�ȭ�ϴ� ��ɾ �ֱ� ���� Ŀ�ǵ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCamera = std::make_unique<Camera>();
	
	for (int i = 0; i < mFrameCount; i++)
		mFrameResources[i] = make_unique<FrameResource>(

			);
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
				// TODO: ���ο� ���� �����ϰ��� Ŀ�ǵ� ť�� ��ɵ��� �Űܾ� �ϱ�
				// ������ �׽�Ʈ�� �ʿ��ϴ�.
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

	if (!mScenes.empty()) mScenes.top()->Draw(mCommandList.Get(), timer);

	// ȭ�� ������ ���¸� �ٽ� PRESENT ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &Extension::ResourceBarrier(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	ThrowIfFailed(mSwapChain->Present(0, 0));  // ȭ����۸� Swap�Ѵ�.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex(); // ���� �ĸ������ �ε����� �����Ѵ�.

	// WaitForGPUComplete();
	
	// ��Ÿ�� ������ ������Ű�� ���� �������ڿ��� ��Ÿ���� �����Ѵ�.
	mCurrFrameResource->mFence = ++mCurrentFenceValue;

	// Ŀ�ǵ� ť�� ��Ÿ�� ������ �����ϴ� ����� �߰��Ѵ�.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFenceValue));
}
