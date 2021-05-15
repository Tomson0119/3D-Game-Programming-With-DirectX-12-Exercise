#pragma once

#include "basewin.h"
#include "gameTimer.h"
#include "dxException.h"

class D3DFramework : public BaseWin<D3DFramework>
{
public:
	D3DFramework();
	D3DFramework(const D3DFramework& rhs) = delete;
	D3DFramework& operator=(const D3DFramework& rhs) = delete;
	virtual ~D3DFramework();

	bool InitFramework();
	void Run();

private:
	bool InitDirect3D();

	void CreateD3DDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvDsvDescriptorHeaps();

	void WaitUntilGPUComplete();
	
	void OutputAdapterInfo(IDXGIAdapter1* adapter);
	void QueryVideoMemory(IDXGIAdapter1* adapter);

	void UpdateFrameStates();

public:
	// BaseWin 오버라이딩 함수
	virtual PCWSTR ClassName() const { return L"Main Window"; }
	virtual LRESULT OnProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Update() { }
	virtual void Draw() { }

private:
	GameTimer mTimer;

	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<ID3D12Device> mD3dDevice;
	
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;

	ComPtr<ID3D12Fence> mFence;
	UINT mCurrentFenceValue = 0;
	HANDLE mFenceEvent;

	static const UINT mSwapChainBufferCount = 2;

	UINT mMsaa4xQualityLevels = 0;
	bool mMsaa4xEnable = false;

	std::wstring mWndCaption = L"D3D12 App";

	int mClientWidth = 800;
	int mClientHeight = 600;
};