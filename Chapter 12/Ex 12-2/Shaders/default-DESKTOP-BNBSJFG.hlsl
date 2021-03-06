
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
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
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

	// Just pass it over to geometry shader.
	vout.PosL = vin.PosL;
	vout.NormalL = vin.NormalL;
	vout.TexC = vin.TexC;

	return vout;	
}

VertexOut midPoint(VertexOut a, VertexOut b)
{
	VertexOut mid;

	float radius = length(a.PosL);

	float3 n = normalize(0.5f * (a.PosL + b.PosL));

	mid.PosL = radius * n;
	mid.NormalL = n;
	mid.TexC = 0.5f * (a.TexC + b.TexC);

	return mid;
}

void subdivide(VertexOut v0, VertexOut v1, VertexOut v2, inout TriangleStream<GeoOut> triStream)
{
	VertexOut v[6];

	v[0] = v0;
	v[1] = midPoint(v0, v1);
	v[2] = midPoint(v0, v2);
	v[3] = midPoint(v1, v2);
	v[4] = v2;
	v[5] = v1;

	GeoOut gout[6];

	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		float4 posW = mul(float4(v[i].PosL, 1.0f), gWorld);
		float3 normalW = mul(v[i].NormalL, (float3x3)gWorld);

		gout[i].PosH = mul(posW, gViewProj);
		gout[i].PosW = posW.xyz;
		gout[i].NormalW = normalW;
		gout[i].TexC = v[i].TexC;
	}

	[unroll]
	for (i = 0; i < 5; ++i)
		triStream.Append(gout[i]);

	triStream.RestartStrip();

	triStream.Append(gout[5]);
	triStream.Append(gout[3]);
	triStream.Append(gout[1]);

	triStream.RestartStrip();
}

// Expand one triangle into 4 triangles (6 vertices).
[maxvertexcount(36)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream)
{
	float distance = length(gEyePosW - float3(gWorld._41, gWorld._42, gWorld._43));
	
	if (distance >= 55)
		// just pass it over ( 3 vertices )
	{
		GeoOut gout;

		for (int i = 0; i < 3; ++i)
		{
			float4 posW = mul(float4(gin[i].PosL, 1.0f), gWorld);
			float3 normalW = mul(gin[i].NormalL, (float3x3)gWorld);

			gout.PosH = mul(posW, gViewProj);
			gout.PosW = posW.xyz;
			gout.NormalW = normalW;
			gout.TexC = gin[i].TexC;

			triStream.Append(gout);
		}
	}
	else if (distance >= 35)
		// subdivide one ( 9 vertices )
		subdivide(gin[0], gin[1], gin[2], triStream);
	else
		// subdivide two ( 36 vertices )
	{
		VertexOut v[6];

		v[0] = gin[0];
		v[1] = midPoint(gin[0], gin[1]);
		v[2] = midPoint(gin[0], gin[2]);
		v[3] = midPoint(gin[1], gin[2]);
		v[4] = gin[2];
		v[5] = gin[1];

		subdivide(v[0], v[1], v[2], triStream);
		subdivide(v[2], v[3], v[4], triStream);
		subdivide(v[1], v[3], v[2], triStream);
		subdivide(v[1], v[5], v[3], triStream);
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