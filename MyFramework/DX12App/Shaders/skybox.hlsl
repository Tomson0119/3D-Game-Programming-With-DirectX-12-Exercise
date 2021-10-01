#include "lighting.hlsl"

Texture2DArray gTexture : register(t0);
SamplerState gWrapSamplerState  : register(s0);
SamplerState gClampSamplerState : register(s1);

cbuffer CameraCB : register(b0)
{
    matrix gView	  : packoffset(c0);
    matrix gProj	  : packoffset(c4);
    matrix gViewProj  : packoffset(c8);    
    float3 gCameraPos : packoffset(c12);
    float gAspect     : packoffset(c12.w);
}

cbuffer LightCB : register(b1)
{
    float4 gAmbient           : packoffset(c0);
    Light gLights[NUM_LIGHTS] : packoffset(c1);
}

cbuffer ObjectCB : register(b2)
{
    matrix gWorld : packoffset(c0);    
    Material gMat : packoffset(c4);
}

struct VertexIn
{
	float3 PosL		: POSITION;
    float3 NormalL	: NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TexCoord = vin.TexCoord;
	
	return vout;
}

float4 PS(VertexOut pin, uint primID : SV_PrimitiveID) : SV_Target
{
    float3 uvw = float3(pin.TexCoord, primID / 2);
    float4 diffuse = gTexture.Sample(gClampSamplerState, uvw) * gMat.Diffuse;
    
    return diffuse;
}