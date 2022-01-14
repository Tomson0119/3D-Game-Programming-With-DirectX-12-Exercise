#include "stdafx.h"
#include "d3dFramework.h"

D3DFramework::D3DFramework()
	: mViewPort{ }, mScissorRect{ }, mFenceValues{ }
{
}

D3DFramework::~D3DFramework()
{
	//if(mD3dDevice) WaitUntilGPUComplete();
	if (mFenceEvent) CloseHandle(mFenceEvent);
}

bool D3DFramework::InitFramework()
{
	if(!InitWindow(mWndCaption.c_str(), gFrameWidth, gFrameHeight))
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
			
			if (!mPaused) {
				Update();
				Draw();
			}
			else
			{
				Sleep(100);
			}
		}
	}
}

void D3DFramework::SetResolution(int width, int height)
{
	gFrameWidth = width;
	gFrameHeight = height;
}

bool D3DFramework::InitDirect3D()
{
	CreateD3DDevice();
	CreateCommandObjects();
	CreateRtvDsvDescriptorHeaps();
	CreateSwapChain();

	OnResize();

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
	for (int i = 0; i < mSwapChainBufferCount; i++) mFenceValues[i] = 1;
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
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;	
	swapChainDesc.BufferDesc.Width = gFrameWidth;
	swapChainDesc.BufferDesc.Height = gFrameHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = mSwapChainBufferCount;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = m_hwnd;
	swapChainDesc.SampleDesc.Count = (mMsaa4xEnable) ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = (mMsaa4xEnable) ? (mMsaa4xQualityLevels - 1) : 0;
	swapChainDesc.Windowed = true;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(mDxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&swapChainDesc,
		(IDXGISwapChain**)mSwapChain.GetAddressOf()));

	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	// Alt-Enter 단축키 사용을 금지한다.
	ThrowIfFailed(mDxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
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

	// For Depth Stencil Descriptor Heap
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	
	ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		IID_PPV_ARGS(&mDsvDescriptorHeap)));
	
	gRtvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	gDsvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	gCbvSrvUavDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DFramework::OnResize()
{
	// 사이즈를 변경하기 전에 명령 목록을 비운다.
	WaitUntilGPUComplete();

	// 커맨드 리스트를 초기화하고, 리소스들의 Com 포인터를 Release한다.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
	for (int i = 0; i < mSwapChainBufferCount; ++i)
		mSwapChainBuffers[i].Reset();
	
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		mSwapChainBufferCount,
		gFrameWidth, gFrameHeight,
		mSwapChainBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
	CreateDepthStencilView();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

	WaitUntilGPUComplete();

	mViewPort.Width = (float)gFrameWidth;
	mViewPort.Height = (float)gFrameHeight;
	mViewPort.TopLeftX = 0.0f;
	mViewPort.TopLeftY = 0.0f;
	mViewPort.MaxDepth = 1.0f;
	mViewPort.MinDepth = 0.0f;

	mScissorRect = { 0, 0, gFrameWidth, gFrameHeight };
}

void D3DFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(CurrentBackBufferView());
	for (UINT i = 0; i < mSwapChainBufferCount; ++i)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffers[i])));
		mD3dDevice->CreateRenderTargetView(mSwapChainBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += gRtvDescriptorSize;
	}
}

void D3DFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = gFrameWidth;
	resourceDesc.Height = gFrameHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;

	resourceDesc.Format = mDepthStencilBufferFormat;
	resourceDesc.SampleDesc.Count = mMsaa4xEnable ? 4 : 1;
	resourceDesc.SampleDesc.Quality = mMsaa4xEnable ? (mMsaa4xQualityLevels - 1) : 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mDepthStencilBufferFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProperties = Extension::HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(mD3dDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&mDepthStencilBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = mDepthStencilBufferFormat;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
	
	mD3dDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(),
		&depthStencilDesc,
		DepthStencilView());
}

void D3DFramework::WaitUntilGPUComplete()
{
	UINT64 currFenceValue = ++mFenceValues[mCurrBackBufferIndex];
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currFenceValue));
	if (mFence->GetCompletedValue() < currFenceValue)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(currFenceValue, mFenceEvent));
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
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mPaused = true;
			mTimer.Stop();
		}
		else
		{
			mPaused = false;
			mTimer.Start();
		}
		break;
		
	case WM_SIZE:  // 윈도우 창의 크기를 변경했을 때
		gFrameWidth = LOWORD(lParam);
		gFrameHeight = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED)
		{
			mPaused = true;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			mPaused = false;
			OnResize();
		}
		else if (wParam == SIZE_RESTORED)
		{
			if (mD3dDevice) {
				mPaused = false;
				OnResize();
			}
		}		
		break;

	case WM_ENTERSIZEMOVE:  // 윈도우 창의 크기 조절 바를 클릭했을 때
		mPaused = true;
		mTimer.Stop();
		break;

	case WM_EXITSIZEMOVE:  // 윈도우 창 크기 조절을 끝마쳤을 때
		mPaused = false;
		mTimer.Start();
		OnResize();
		break;

	case WM_GETMINMAXINFO:  // 윈도우의 최대 최소 크기를 지정한다.
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = { 200, 200 };
		break;
	
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnProcessMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnProcessMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_MOUSEMOVE:
		OnProcessMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
	return 0;
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

void D3DFramework::ChangeFullScreenState()
{
	HWND display = GetDesktopWindow();
	RECT rect{ };

	GetWindowRect(display, &rect);
	gFrameWidth = rect.right - rect.left;
	gFrameHeight = rect.bottom - rect.top;

	OnResize();

	mSwapChain->GetFullscreenState(&mFullScreen, NULL);
	mSwapChain->SetFullscreenState(!mFullScreen, NULL);

	OnResize();
}

ID3D12Resource* D3DFramework::CurrentBackBuffer() const
{
	return mSwapChainBuffers[mCurrBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DFramework::CurrentBackBufferView() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += mCurrBackBufferIndex * gRtvDescriptorSize;
	return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DFramework::DepthStencilView() const
{
	return mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
