#pragma once

#ifndef _DEBUG
#define _DEBUG
#else
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <dxgidebug.h>
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


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")


// C++ 헤더 파일:
#include <array>
#include <vector>
#include <stack>
#include <unordered_map>
#include <string>
#include <memory>

#include "dxException.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

extern ComPtr<ID3D12Resource> CreateBufferResource(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer);

extern ComPtr<ID3DBlob> CompileShader(
	const std::wstring& fileName,
	const std::string& entry,
	const std::string& target,
	const D3D_SHADER_MACRO* defines = nullptr);


inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(f)												\
{																		\
	HRESULT hr_ = (f);													\
	std::wstring wfn = AnsiToWString(__FILE__);							\
	if (FAILED(hr_))	{ throw DxException(hr_, L#f, wfn, __LINE__); } \
}																		
#endif


////////////////////////////////////////////////////////////////////////////
//
namespace Math
{
	const float PI = 3.1415926535f;
}

namespace Vector3
{
	inline XMFLOAT3 VectorToFloat3(FXMVECTOR& vector)
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, vector);
		return ret;
	}

	inline XMFLOAT3 Replicate(float value)
	{
		return VectorToFloat3(XMVectorReplicate(value));
	}

	inline XMFLOAT3 MultiplyAdd(float delta, XMFLOAT3& src, XMFLOAT3& dst)
	{		
		XMVECTOR v1 = XMLoadFloat3(&Replicate(delta));
		XMVECTOR v2 = XMLoadFloat3(&src);
		XMVECTOR v3 = XMLoadFloat3(&dst);
		return VectorToFloat3(XMVectorMultiplyAdd(v1, v2, v3));
	}

	inline XMFLOAT3 TransformNormal(XMFLOAT3& src, FXMMATRIX& mat)
	{
		return VectorToFloat3(XMVector3TransformNormal(XMLoadFloat3(&src), mat));
	}

	inline XMFLOAT3 Normalize(XMFLOAT3& v)
	{
		return VectorToFloat3(XMVector3Normalize(XMLoadFloat3(&v)));
	}

	inline XMFLOAT3 Subtract(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline XMFLOAT3 Cross(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVector3Cross(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline float Dot(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}
}

namespace Matrix4x4
{
	inline XMFLOAT4X4 Identity4x4()
	{	
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixIdentity());
		return ret;
	}
}