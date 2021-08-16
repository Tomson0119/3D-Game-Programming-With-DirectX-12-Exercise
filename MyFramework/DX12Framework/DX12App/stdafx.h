#pragma once

#include "../DX12Common/constantBuffer.h"
#include "../DX12Common/dxException.h"
#include "../DX12Common/d3dFramework.h"

#include "../DX12Common/stdafx.h"


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