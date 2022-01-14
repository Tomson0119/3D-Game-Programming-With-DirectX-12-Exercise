#include "common.hlsl"

Texture2D gDiffuseMap : register(t0);

struct VertexIn
{
	float3 PosL		: POSITION;
    float3 NormalL	: NORMAL;
    float3 TangentL : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION0;
    float4 PosS     : POSITION1;
    float3 NormalW  : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    vout.PosS = mul(posW, gShadowTransform);
    
    float4x4 tWorld = transpose(gWorld);
    vout.NormalW = mul((float3x3)tWorld, vin.NormalL);
    vout.TangentW = mul((float3x3)tWorld, vin.TangentL);
    
    vout.TexCoord = vin.TexCoord;
	
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{  
    float4 diffuse = gDiffuseMap.Sample(gAnisotropicWrap, pin.TexCoord) * gMat.Diffuse;
    
    float3 view = normalize(gCameraPos - pin.PosW);
    float4 ambient = gAmbient * diffuse;
    
    float shadowFactor[3] = { 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < 3; i++)
    {
        shadowFactor[i] = CalcShadowFactor(pin.PosS);
    }
    
    Material mat = { diffuse, gMat.Fresnel, gMat.Roughness };
    float4 directLight = ComputeLighting(gLights, mat, pin.NormalW, view, shadowFactor);
    
    float4 result = ambient + directLight;
    result.a = diffuse.a;
    return result;
}