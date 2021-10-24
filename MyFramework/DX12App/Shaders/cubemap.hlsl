#include "common.hlsl"

TextureCube gCubeMap : register(t0);

struct VertexIn
{
	float3 PosL		: POSITION;
    float3 NormalL	: NORMAL;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
    
    float4x4 tWorld = transpose(gWorld);
    vout.NormalW = mul((float3x3)tWorld, vin.NormalL);
	
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{  
    pin.NormalW = normalize(pin.NormalW);
    
    float3 fromEye = normalize(pin.PosW - gCameraPos.xyz);
    float3 reflected = normalize(reflect(fromEye, pin.NormalW));
    float4 result = gCubeMap.Sample(gLinearWrap, reflected);
    
    return result;
}