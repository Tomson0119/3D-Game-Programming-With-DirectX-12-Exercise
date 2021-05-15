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
	
	void LogAdapters(std::vector<IDXGIAdapter1*>& adapters);

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

	std::wstring mWndCaption = L"D3D12 App";

	int mClientWidth = 800;
	int mClientHeight = 600;
};