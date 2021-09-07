#include "lighting.hlsl"

Texture2D gTexture : register(t0);
SamplerState gSamplerState : register(s0);

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

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuse = gTexture.Sample(gSamplerState, pin.TexCoord) * gMat.Diffuse;
    
    pin.NormalW = normalize(pin.NormalW);
    
    float3 view = normalize(gCameraPos - pin.PosW);
    float4 ambient = gAmbient * diffuse;
    
    Material mat = { diffuse, gMat.Fresnel, gMat.Roughness };
    float4 directLight = ComputeLighting(gLights, mat, pin.NormalW, view);
    
    float4 result = ambient + directLight;
    result.a = gMat.Diffuse.a;
    
    return ambient;
}