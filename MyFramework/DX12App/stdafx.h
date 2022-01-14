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
#include "DDSTextureLoader12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")


// C++ 헤더 파일:
#include <array>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <fstream>
#include <iostream>
#include <cmath>
#include <chrono>


#include "d3dExtension.h"
#include "dxException.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

using namespace std::chrono_literals;

extern UINT gRtvDescriptorSize;
extern UINT gDsvDescriptorSize;
extern UINT gCbvSrvUavDescriptorSize;

extern int gFrameWidth;
extern int gFrameHeight;

extern ComPtr<ID3D12Resource> CreateBufferResource(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer,
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

extern ComPtr<ID3D12Resource> CreateTexture2DResource(
	ID3D12Device* device,
	UINT width, UINT height, UINT elements, UINT miplevels,
	DXGI_FORMAT format, D3D12_RESOURCE_FLAGS resourceFlags,
	D3D12_RESOURCE_STATES resourceStates, D3D12_CLEAR_VALUE* clearValue);

inline UINT GetConstantBufferSize(UINT bytes)
{
	return ((bytes + 255) & ~255);
}

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

#define POINT_LIGHT		  1
#define SPOT_LIGHT		  2
#define DIRECTIONAL_LIGHT 3

struct LightInfo
{
	XMFLOAT3 Diffuse = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float    padding0;
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float	 padding1;
	XMFLOAT3 Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float	 padding2;
	float    Range;
	int		 Type;
	
	void SetInfo(
		const XMFLOAT3& diffuse,
		const XMFLOAT3& position,
		const XMFLOAT3& direction,
		float range, int type)
	{
		Diffuse = diffuse;
		Position = position;
		Direction = direction;
		Range = range;
		Type = type;
	}
};

struct LightConstants
{
	XMFLOAT4X4 ShadowTransform;
	XMFLOAT4 Ambient;
	LightInfo Lights[NUM_LIGHTS];
};

struct CameraConstants
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Proj;
	XMFLOAT4X4 ViewProj;
	XMFLOAT3 CameraPos;
	float Aspect;
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

struct GameInfoConstants
{
	XMFLOAT4 RandFloat4;
	XMFLOAT3 PlayerPosition;
	UINT KeyInput;
	float CurrentTime;
	float ElapsedTime;
};


////////////////////////////////////////////////////////////////////////////
//
namespace Math
{
	const float PI = 3.1415926535f;

	inline int RandInt(int min, int max)
	{
		return rand() % (max - min + 1) + min;
	}

	inline float RandFloat(float min, float max)
	{
		float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		return r * (max - min) + min;
	}

	inline float ClampFloat(float x, float min, float max)
	{
		return (x < min) ? min : ((x > max) ? max : x);
	}
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

	inline XMFLOAT3 Multiply(float scalar, XMFLOAT3& v)
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, scalar * XMLoadFloat3(&v));
		return ret;
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

	inline XMFLOAT3 Transform(XMFLOAT3& src, FXMMATRIX& mat)
	{
		return VectorToFloat3(XMVector3Transform(XMLoadFloat3(&src), mat));
	}

	inline XMFLOAT3 TransformCoord(XMFLOAT3& src, XMFLOAT4X4& mat)
	{
		return VectorToFloat3(XMVector3TransformCoord(XMLoadFloat3(&src), XMLoadFloat4x4(&mat)));
	}

	inline XMFLOAT3 Normalize(XMFLOAT3& v)
	{
		return VectorToFloat3(XMVector3Normalize(XMLoadFloat3(&v)));
	}

	inline XMFLOAT3 Subtract(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline XMFLOAT3 ScalarProduct(XMFLOAT3& v, float scalar)
	{
		return VectorToFloat3(XMLoadFloat3(&v) * scalar);
	}

	inline XMFLOAT3 Cross(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVector3Cross(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline float Length(XMFLOAT3& v)
	{
		XMFLOAT3 ret = VectorToFloat3(XMVector3Length(XMLoadFloat3(&v)));
		return ret.x;
	}

	inline float Dot(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline XMFLOAT3 Add(XMFLOAT3& v, float value)
	{
		return VectorToFloat3(XMVectorAdd(XMLoadFloat3(&v), XMVectorReplicate(value)));
	}

	inline XMFLOAT3 Add(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return VectorToFloat3(XMLoadFloat3(&v1) + XMLoadFloat3(&v2));
	}

	inline XMFLOAT3 Add(XMFLOAT3& v1, XMFLOAT3& v2, float distance)
	{
		return VectorToFloat3(XMLoadFloat3(&v1) + XMLoadFloat3(&v2) * distance);
	}

	inline XMFLOAT3 ClampFloat3(XMFLOAT3& input, XMFLOAT3& min, XMFLOAT3& max)
	{
		XMFLOAT3 ret;
		ret.x = (min.x > input.x) ? min.x : ((max.x < input.x) ? max.x : input.x);
		ret.y = (min.y > input.y) ? min.y : ((max.y < input.y) ? max.y : input.y);
		ret.z = (min.z > input.z) ? min.z : ((max.z < input.z) ? max.z : input.z);
		return ret;
	}

	inline XMFLOAT3 Absf(XMFLOAT3& v)
	{
		return XMFLOAT3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
	}

	inline bool Equal(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return XMVector3Equal(XMLoadFloat3(&v1), XMLoadFloat3(&v2));
	}

	inline bool Less(XMFLOAT3& v, float x)
	{
		return XMVector3Less(XMLoadFloat3(&v), XMVectorReplicate(x));
	}
}

namespace Vector4
{
	inline XMFLOAT4 Zero()
	{
		return XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	inline bool Equal(XMFLOAT4& v1, XMFLOAT4& v2)
	{
		return XMVector4Equal(XMLoadFloat4(&v1), XMLoadFloat4(&v2));
	}

	inline XMFLOAT4 Add(XMFLOAT4& v1, XMFLOAT4& v2)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, XMVectorAdd(XMLoadFloat4(&v1), XMLoadFloat4(&v2)));
		return ret;
	}

	inline XMFLOAT4 Multiply(float scalar, XMFLOAT4& v)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, scalar * XMLoadFloat4(&v));
		return ret;
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

	inline XMFLOAT4X4 Multiply(FXMMATRIX& mat1, XMFLOAT4X4& mat2)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, mat1 * XMLoadFloat4x4(&mat2));
		return ret;
	}

	inline XMFLOAT4X4 Reflect(XMFLOAT4& plane)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixReflect(XMLoadFloat4(&plane)));
		return ret;
	}
}