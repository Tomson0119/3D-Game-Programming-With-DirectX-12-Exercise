#include "common.hlsl"

struct VertexIn
{
    float3 PosL     : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

struct DepthOut
{
    float Position : SV_Target;
    float Depth : SV_Depth;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    float4 PosW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(PosW, gViewProj);    
    vout.TexCoord = vin.TexCoord;
	
    return vout;
}

DepthOut PS(VertexOut pin)
{
    DepthOut dout = (DepthOut)0;
    dout.Position = pin.PosH.z;
    dout.Depth = pin.PosH.z;
    return dout;
}