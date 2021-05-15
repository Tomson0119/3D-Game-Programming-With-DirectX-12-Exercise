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
		OutputAdapterInfo(adapter1);

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

	QueryVideoMemory(adapter1);
	if (adapter1) adapter1->Release();

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(mD3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMsaaQualityLevels,
		sizeof(d3dMsaaQualityLevels)));

	mMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	mMsaa4xEnable = (mMsaa4xQualityLevels > 1) ? true : false;

	ThrowIfFailed(mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void D3DFramework::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc = {};
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(mD3dDevice->CreateCommandQueue(
		&d3dCommandQueueDesc, 
		IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(mD3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&mCommandAllocator)));

	ThrowIfFailed(mD3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCommandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&mCommandList)));

	mCommandList->Close();
}

void D3DFramework::CreateSwapChain()
{
	


}

void D3DFramework::CreateRtvDsvDescriptorHeaps()
{
	// For Render Target Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc = {};
	d3dDescriptorHeapDesc.NumDescriptors = mSwapChainBufferCount;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.NodeMask = 0;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		IID_PPV_ARGS(&mRtvDescriptorHeap)));

	mRtvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// For Depth Stencil Descriptor Heap
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	
	ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		IID_PPV_ARGS(&mDsvDescriptorHeap)));
	
	mDsvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void D3DFramework::WaitUntilGPUComplete()
{
	++mCurrentFenceValue;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFenceValue));
	if (mFence->GetCompletedValue() < mCurrentFenceValue)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFenceValue, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void D3DFramework::OutputAdapterInfo(IDXGIAdapter1* adapter)
{
	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);

	std::wstring text = L"----- Adapter : ";
	text += desc.Description;
	text += L"\n";

	OutputDebugString(text.c_str());
}

void D3DFramework::QueryVideoMemory(IDXGIAdapter1* adapter)
{
	IDXGIAdapter3* adapter3 = nullptr;
	ThrowIfFailed(adapter->QueryInterface(&adapter3));
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