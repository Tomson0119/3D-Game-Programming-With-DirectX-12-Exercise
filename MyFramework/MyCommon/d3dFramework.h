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

	virtual bool InitFramework();
	void Run();

private:
	bool InitDirect3D();

	void CreateD3DDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();	
	
	void OutputAdapterInfo(IDXGIAdapter1* adapter);
	void QueryVideoMemory(IDXGIAdapter1* adapter);

	void UpdateFrameStates();

protected:
	void WaitUntilGPUComplete();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

public:
	// BaseWin 오버라이딩 함수
	virtual PCWSTR ClassName() const { return L"Main Window"; }
	virtual LRESULT OnProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void OnResize();

	virtual void OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Update(const GameTimer& timer);
	virtual void Draw(const GameTimer& timer) = 0;

protected:
	GameTimer mTimer;

	bool mPaused     = false;
	BOOL mFullScreen = false;

	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<ID3D12Device>  mD3dDevice;
	
	ComPtr<ID3D12CommandQueue>		  mCommandQueue;
	ComPtr<ID3D12CommandAllocator>	  mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap;

	static const UINT mSwapChainBufferCount = 2;
	ComPtr<ID3D12Resource> mSwapChainBuffers[mSwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	ComPtr<ID3D12Fence> mFence;
	UINT mCurrentFenceValue = 0;
	HANDLE mFenceEvent = NULL;

	ComPtr<IDXGISwapChain3> mSwapChain;
	UINT mCurrBackBufferIndex = 0;

	UINT mMsaa4xQualityLevels = 0;
	bool mMsaa4xEnable = false;

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;

	DXGI_FORMAT mSwapChainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::wstring mWndCaption = L"D3D12 App";

	int mFrameWidth = 800;
	int mFrameHeight = 600;
};