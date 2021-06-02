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
#include <windowsx.h>
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
#include <fstream>
#include <math.h>

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
#define NUM_LIGHTS 3

struct Light
{
	XMFLOAT3A Position = XMFLOAT3A(0.0f, 0.0f, 0.0f);
	XMFLOAT3A Direction = XMFLOAT3A(0.0f, 0.0f, 0.0f);
	XMFLOAT3A Diffuse = XMFLOAT3A(0.0f, 0.0f, 0.0f);
};

struct LightConstants
{
	XMFLOAT4 Ambient;
	Light Lights[NUM_LIGHTS];
};

struct CameraConstants
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Proj;
	XMFLOAT4X4 ViewProj;
	XMFLOAT3 CameraPos;
};

struct Material
{
	XMFLOAT4 Color;
	XMFLOAT3 Frenel;
	float Roughness;
};

struct ObjectConstants
{
	XMFLOAT4X4 World;
	Material Mat;
};


////////////////////////////////////////////////////////////////////////////
//
namespace Math
{
	const float PI = 3.1415926535f;
}

namespace Vector3
{
	inline XMFLOAT3 Zero()
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

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

	inline XMFLOAT3 Add(XMFLOAT3& v1, XMFLOAT3& v2, float distance)
	{
		return VectorToFloat3(XMLoadFloat3(&v1) + XMLoadFloat3(&v2) * distance);
	}
}

namespace Vector4
{
	inline XMFLOAT4 Zero()
	{
		return XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
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

	inline XMFLOAT4X4 Transpose(XMFLOAT4X4& mat)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixTranspose(XMLoadFloat4x4(&mat)));
		return ret;
	}

	inline XMFLOAT4X4 Multiply(XMFLOAT4X4& mat1, XMFLOAT4X4& mat2)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixMultiply(XMLoadFloat4x4(&mat1), XMLoadFloat4x4(&mat2)));
		return ret;
	}

	inline XMFLOAT4X4 Multiply(XMMATRIX& mat1, XMFLOAT4X4& mat2)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixMultiply(mat1, XMLoadFloat4x4(&mat2)));
		return ret;
	}
}