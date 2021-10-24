#include "common.hlsl"

Texture2DArray gTexture : register(t0);

struct VertexIn
{
	float3 PosW  : POSITION;
    float2 Size : SIZE;
};

struct GeoIn
{
    float3 PosW : POSITION;
    float2 Size : SIZE;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
    float2 TexCoord : TEXCOORD;
    uint PrimID     : SV_PrimitiveID;
};

GeoIn VS(VertexIn vin)
{
    GeoIn gout;
    gout.PosW = vin.PosW;
    gout.Size = vin.Size;
    return gout;
}

[maxvertexcount(4)]
void GS(point GeoIn gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<VertexOut> triStream)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gCameraPos - gin[0].PosW;
    
    look.y = 0.0f;
    look = normalize(look);
    
    float3 right = cross(up, look);
    
    float hw = gin[0].Size.x * 0.5f;
    float hh = gin[0].Size.y * 0.5f;
    
    float4 v[4];
    v[0] = float4(gin[0].PosW + hw * right - hh * up, 1.0f);
    v[1] = float4(gin[0].PosW + hw * right + hh * up, 1.0f);
    v[2] = float4(gin[0].PosW - hw * right - hh * up, 1.0f);
    v[3] = float4(gin[0].PosW - hw * right + hh * up, 1.0f);
    
    float2 TexCoord[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    VertexOut vout;
    [unroll]
    for (int i = 0; i < 4;i++)
    {
        vout.PosH = mul(v[i], gViewProj);
        vout.PosW = v[i].xyz;
        vout.NormalW = look;
        vout.TexCoord = TexCoord[i];
        vout.PrimID = primID;
        triStream.Append(vout);
    }
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 uvw = float3(pin.TexCoord, pin.PrimID % 4);
    float4 diffuse = gTexture.Sample(gAnisotropicWrap, uvw) * gMat.Diffuse;
    
    clip(diffuse.a - 0.5f);
    
    pin.NormalW = normalize(pin.NormalW);
    
    float3 view = normalize(gCameraPos - pin.PosW);
    float4 ambient = gAmbient * diffuse;
    
    Material mat = { diffuse, gMat.Fresnel, gMat.Roughness };
    float4 directLight = ComputeLighting(gLights, mat, pin.NormalW, view);
    
    float4 result = ambient + directLight;
    result.a = gMat.Diffuse.a;
    
    return result;
}