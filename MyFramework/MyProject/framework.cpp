#include "../MyCommon/stdafx.h"
#include "framework.h"

GameFramework::GameFramework()
	: D3DFramework()
{
}

GameFramework::~GameFramework()
{
}

bool GameFramework::InitFramework()
{
	if (!D3DFramework::InitFramework())
		return false;

	// �ʱ�ȭ�ϴ� ��ɾ �ֱ� ���� Ŀ�ǵ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// Build Objects
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndLayouts();
	BuildMeshObjects();
	BuildPSO();	

	// Command List�� �ݰ� Queue�� ��ɾ �ƴ´�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	// �ʱ�ȭ�� ����� ������ ��ٸ���.
	WaitUntilGPUComplete();

	return true;
}

void GameFramework::BuildRootSignature()
{
}

void GameFramework::BuildDescriptorHeaps()
{
}

void GameFramework::BuildShadersAndLayouts()
{
}

void GameFramework::BuildMeshObjects()
{
}

void GameFramework::BuildPSO()
{
}

void GameFramework::Update(const GameTimer& timer)
{
	D3DFramework::Update(timer);
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
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// ȭ�� ���ۿ� ���� ���ٽ� ���۸� �ʱ�ȭ�Ѵ�.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// �������� ���۸� ��ü������ �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), TRUE, &DepthStencilView());

	// ��� ���� �����ڸ� �����Ѵ�.
	//
	//

	// Root signature�� �����Ѵ�.
	//
	//

	// ������ �ε��� ����, �������� �����Ѵ�.
	//
	//
	// 
	// Root �����ڸ� �����Ѵ�.
	//
	//

	// ������ ȭ�鿡 �׸���.
	//
	//

	// ȭ�� ������ ���¸� �ٽ� PRESENT ���·� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	ThrowIfFailed(mSwapChain->Present(0, 0));  // ȭ����۸� Swap�Ѵ�.
	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	WaitUntilGPUComplete();
}


