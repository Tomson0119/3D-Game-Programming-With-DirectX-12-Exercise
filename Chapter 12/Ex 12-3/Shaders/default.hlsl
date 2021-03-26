
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
	float4   gDiffuseAlbedo;
	float3   gFresnelR0;
	float    gRoughness;
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
	float  cbPerObjectPad1;
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
	float  gFogStart;
	float  gFogRange;
	float2 cbPerObjectPad2;

	Light gLights[MaxLights];
};

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float3 TangentL: TANGENT;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float3 TangentW: TANGENT;
	float2 TexC    : TEXCOORD;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW	   : POSITION;
	float3 NormalW : NORMAL;
	float3 TangentW: TANGENT;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Assume that nonuniform scaling.
	float3 normalW = mul(vin.NormalL, (float3x3)gWorld);
	float3 tangentW = mul(vin.TangentL, (float3x3)gWorld);

	vout.PosW = posW.xyz;
	vout.NormalW = normalW;
	vout.TangentW = tangentW;
	vout.TexC = vin.TexC;

	return vout;	
}

// Expand one triangle into 4 triangles (6 vertices).
[maxvertexcount(3)]
void GS(point VertexOut gin[1], inout TriangleStream<GeoOut> triStream)
{
	float3 T = gin[0].TangentW;
	float3 N = gin[0].NormalW;
	float3 B = cross(N, T);

	float4 v[3];
	v[0] = float4(gin[0].PosW - 1.0f * B, 1.0f);
	v[1] = float4(gin[0].PosW + 0.5f * B + 0.86f * T, 1.0f);
	v[2] = float4(gin[0].PosW + 0.5f * B - 0.86f * T, 1.0f);

	GeoOut gout;

	[unroll]
	for (int i = 0; i < 3; ++i)
	{
		float4 pos = mul(v[i], gViewProj);

		gout.PosH = pos;
		gout.PosW = pos.xyz;
		gout.NormalW = gin[0].NormalW;
		gout.TangentW = gin[0].TangentW;
		gout.TexC = gin[0].TexC;

		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1. We do this test as soon
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);

	// Vector from point being lit to eye.
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Indirect lighting.
	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif	

	// Common convention to take alpha from  diffuse material.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}