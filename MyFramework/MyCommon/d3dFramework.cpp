#include "stdafx.h"
#include "d3dFramework.h"

D3DFramework::D3DFramework()
{
}

D3DFramework::~D3DFramework()
{
}

bool D3DFramework::InitFramework()
{
	if(!InitWindow(mWndCaption.c_str(), mClientWidth, mClientHeight))
		return false;

	if (!InitDirect3D())
		return false;

	return true;
}

bool D3DFramework::InitDirect3D()
{
	CreateD3DDevice();
	CreateCommandObjects();
	CreateRtvDsvDescriptorHeaps();
	CreateSwapChain();

	return true;
}

void D3DFramework::CreateD3DDevice()
{
	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	{
		ComPtr<ID3D12Debug> d3dDebugController = nullptr;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebugController)));
		d3dDebugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mDxgiFactory)));

	UINT i = 0;
	IDXGIAdapter1* adapter1 = nullptr;
	while (mDxgiFactory->EnumAdapters1(i++, &adapter1) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 adapter1Desc;
		adapter1->GetDesc1(&adapter1Desc);
		
		if (adapter1Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

		if (SUCCEEDED(D3D12CreateDevice(
			adapter1,
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&mD3dDevice))))
			break;
	}

	if (!adapter1)
	{
		ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));

		ThrowIfFailed(D3D12CreateDevice(
			adapter1,
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&mD3dDevice)));
	}

	IDXGIAdapter3* adapter3 = nullptr;
	ThrowIfFailed(adapter1->QueryInterface(&adapter3));
	if (adapter3)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO localVideoMemoryInfo, nonLocalVideoMemoryInfo;
		ThrowIfFailed(adapter3->QueryVideoMemoryInfo(
			0,
			DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
			&localVideoMemoryInfo));
		ThrowIfFailed(adapter3->QueryVideoMemoryInfo(
			0,
			DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
			&nonLocalVideoMemoryInfo));
		adapter3->Release();
	}
	if (adapter1) adapter1->Release();



}

void D3DFramework::CreateCommandObjects()
{
}

void D3DFramework::CreateSwapChain()
{
}

void D3DFramework::CreateRtvDsvDescriptorHeaps()
{
}

void D3DFramework::LogAdapters(std::vector<IDXGIAdapter1*>& adapters)
{
}

void D3DFramework::Run()
{
	MSG msg{};

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// 윈도우 메세지 처리
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else  // 타이머 및 Framework 업데이트
		{
			mTimer.Tick();
			UpdateFrameStates();
			Update();
			Draw();
		}
	}
}

LRESULT D3DFramework::OnProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:  // 윈도우 창을 활성화 했을 때
		break;
		
	case WM_SIZE:  // 윈도우 창의 크기를 변경했을 때
		break;

	case WM_ENTERSIZEMOVE:  // 윈도우 창의 크기 조절 바를 클릭했을 때
		break;

	case WM_EXITSIZEMOVE:  // 윈도우 창 크기 조절을 끝마쳤을 때
		break;

	case WM_GETMINMAXINFO:  // 윈도우의 최대 최소 크기를 지정한다.
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = { 200, 200 };
		break;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessMouseInput(uMsg, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessKeyInput(uMsg, wParam, lParam);
		break;

	case WM_MENUCHAR:  // 대응하지 않는 단축키를 눌렀을 때
		// 삑 소리를 방지한다.
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}

void D3DFramework::OnProcessMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void D3DFramework::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYUP:
		if ((int)wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	}
}

void D3DFramework::UpdateFrameStates()
{
	static int frameCount = 0;
	static float elapsed = 0.0f;

	frameCount++;

	if (mTimer.TotalTime() - elapsed >= 1.0f)
	{
		float fps = static_cast<float>(frameCount);
		float mspf = 1000.0f / fps;

		std::wstring new_caption = mWndCaption +
			L"    FPS : " + std::to_wstring(fps) +
			L"   mspf : " + std::to_wstring(mspf);

		SetWindowText(m_hwnd, new_caption.c_str());

		frameCount = 0;
		elapsed += 1.0f;
	}
}