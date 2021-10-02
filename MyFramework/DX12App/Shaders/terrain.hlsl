#include "lighting.hlsl"

Texture2D gBaseTexture     : register(t0);
Texture2D gDetailedTexture : register(t1);
Texture2D gRoadTexture     : register(t2);
    
SamplerState gSamplerState : register(s0);

cbuffer CameraCB : register(b0)
{
    matrix gView      : packoffset(c0);
    matrix gProj      : packoffset(c4);
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
    float3 PosL      : POSITION;
    float3 NormalL   : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct VertexOut
{
    float4 PosH      : SV_POSITION;
    float3 PosW      : POSITION;
    float3 NormalW   : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
    float3x3 tWorld = transpose(gWorld);
    vout.NormalW = mul(tWorld, vin.NormalL);
    vout.TexCoord0 = vin.TexCoord0;
    vout.TexCoord1 = vin.TexCoord1;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 baseTexDiffuse = gBaseTexture.Sample(gSamplerState, pin.TexCoord0) * gMat.Diffuse;    
    float4 detailedTexDiffuse = gDetailedTexture.Sample(gSamplerState, pin.TexCoord1) * gMat.Diffuse;
    float4 roadTexDiffuse = gRoadTexture.Sample(gSamplerState, pin.TexCoord1) * gMat.Diffuse;    
    
    float4 finalDiffuse = saturate(baseTexDiffuse * 0.7f + detailedTexDiffuse * 0.3f);
    pin.NormalW = normalize(pin.NormalW);
    
    float3 view = normalize(gCameraPos - pin.PosW);
    float4 ambient = gAmbient * finalDiffuse;
    
    Material mat = { finalDiffuse, gMat.Fresnel, gMat.Roughness };
    float4 directLight = ComputeLighting(gLights, mat, pin.NormalW, view);
    
    float4 result = finalDiffuse + directLight;
    result.a = gMat.Diffuse.a;
    
    return result;
}