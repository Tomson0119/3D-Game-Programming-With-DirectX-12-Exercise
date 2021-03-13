
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "lightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap		  : register(s0);
SamplerState gsamPointClamp		  : register(s1);
SamplerState gsamLinearWrap		  : register(s2);
SamplerState gsamLinearClamp	  : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMatTransform;
};

cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;

	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;

	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;

	float4 gAmbientLight;

	// Allow application to change fog parameters once per frame.
	// For example.=, we may only use fog for certain times of day.
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPerObjectPad2;

	Light gLights[MaxLights];
};

// Stencil Ref Constants.
cbuffer gStencilConstants : register(b3)
{
	uint gStencil;
};

struct VertexIn
{
	float3 PosL : POSITION;
	float2 TexC : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD;
};

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.TexC = gTexCoords[vid];
	vout.PosH = float4(2.0f * vout.TexC.x - 1.0f,
		1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 red = float4(1.0f, 0.0f, 0.0f, 1.0f);
	float4 green = float4(0.0f, 1.0f, 0.0f, 1.0f);
	float4 blue = float4(0.0f, 0.0f, 1.0f, 1.0f);
	float4 purple = float4(0.5f, 0.0f, 0.5f, 1.0f);
	float4 yellow = float4(0.5f, 0.5f, 0.0f, 1.0f);

	if (gStencil == 1)
		return green;
	else if (gStencil == 2)
		return blue;
	else if (gStencil == 3)
		return yellow;
	else if (gStencil == 4)
		return red;
	else
		return purple;
}