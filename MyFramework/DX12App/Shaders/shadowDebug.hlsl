#include "common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    vout.TexCoord = vin.TexCoord;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuse = gShadowMap.Sample(gLinearWrap, pin.TexCoord);
    return float4(diffuse.rrr * 0.8f, 1.0f);
}