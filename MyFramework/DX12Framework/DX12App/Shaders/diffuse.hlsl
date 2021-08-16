#include "lighting.hlsl"

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
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float3 PosW  : POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
    vout.Color = vin.Color;
    
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}