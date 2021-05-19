#pragma once

#ifndef _DEBUG
#define _DEBUG
#endif


// Window 헤더 파일:
#include <Windows.h>
#include <sdkddkver.h>
#include <wrl.h>
#include <comdef.h>


// D3D12 헤더 파일:
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>

#include "d3dx12.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")


// C++ 헤더 파일:
#include <vector>
#include <string>


using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;


inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(f)										   \
{																   \
	HRESULT hr = (f);											   \
	std::wstring wfn = AnsiToWString(__FILE__);					   \
	if (FAILED(hr))	{ throw DxException(hr, L#f, wfn, __LINE__); } \
}																		
#endif