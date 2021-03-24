
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
	float3 PosL : POSITION;
	float2 Size : SIZE;
};

struct VertexOut
{
	float3 PosW : POSITION;
	float2 Size : SIZE;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW	   : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Just pass data over geometry shader.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;
	vout.Size = vin.Size;

	return vout;	
}

// Expand two vertices in line into a quad (4 vertices).
[maxvertexcount(4)]
void GS(line VertexOut gin[2], inout LineStream<GeoOut> lineStream)
{
	float height = gin[0].Size.y;

	float4 vertices[4];
	vertices[0] = float4(gin[0].PosW, 1.0f);
	vertices[1] = float4(gin[1].PosW, 1.0f);
	vertices[2] = float4(gin[1].PosW.x, gin[1].PosW.y + height, gin[1].PosW.z, 1.0f);
	vertices[3] = float4(gin[0].PosW.x, gin[0].PosW.y + height, gin[0].PosW.z, 1.0f);

	// Calculate normal vector facing outside.
	float3 right = (vertices[0] - vertices[1]).xyz;
	float3 up = (vertices[1] - vertices[2]).xyz;
	float3 normal = normalize(cross(up, right));
	
	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.PosH = mul(vertices[i], gViewProj);
		gout.PosW = vertices[i].xyz;
		gout.NormalW = normal;
		gout.TexC = float2(1.0f, 1.0f); // Since polygon is line strip, 
										// the texture coordinates are pointless..
		lineStream.Append(gout);
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