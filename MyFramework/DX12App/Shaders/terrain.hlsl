#include "common.hlsl"

Texture2D gBaseTexture     : register(t0);
Texture2D gDetailedTexture : register(t1);
Texture2D gRoadTexture     : register(t2);
Texture2D gHeightmap       : register(t3);
Texture2D gNormalmap       : register(t4);

struct VertexIn
{
    float3 PosL      : POSITION;
    float3 NormalL   : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct VertexOut
{
    float3 PosL      : POSITION0;
    float3 PosW      : POSITION1;
    float3 NormalL   : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct HsConstant
{
    float TessEdges[4] : SV_TessFactor;
    float TessInsides[2] : SV_InsideTessFactor;
};

struct HsOut
{
    float3 PosL      : POSITION;
    float3 NormalL   : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct DsOut
{
    float4 PosH         : SV_POSITION;
    float3 PosW         : POSITION0;
    float4 PosS         : POSITION1;
    float3 NormalW      : NORMAL;
    float2 TexCoord0    : TEXCOORD0;
    float2 TexCoord1    : TEXCOORD1;
    float4 Tessellation : TEXCOORD2;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalL = vin.NormalL;
    vout.TexCoord0 = vin.TexCoord0;
    vout.TexCoord1 = vin.TexCoord1;    
    return vout;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(25)]
[patchconstantfunc("HSConstant")]
[maxtessfactor(64.0f)]
HsOut HS(InputPatch<VertexOut, 25> hin, uint i : SV_OutputControlPointID)
{
    HsOut hout;
    hout.PosL = hin[i].PosL;
    hout.NormalL = hin[i].NormalL;
    hout.TexCoord0 = hin[i].TexCoord0;
    hout.TexCoord1 = hin[i].TexCoord1;
    return hout;
}

float CalculateLODTessFactor(float3 pos)
{
    float distToCamera = distance(pos, gCameraPos);
    float s = saturate((distToCamera - 10.0f) / (500.0f - 10.0f));
    return lerp(64.0f, 1.0f, s);
}

HsConstant HSConstant(InputPatch<VertexOut, 25> hin)
{
    HsConstant hconst;
   
    float3 e0 = 0.5f * (hin[0].PosW + hin[4].PosW);
    float3 e1 = 0.5f * (hin[0].PosW + hin[20].PosW);
    float3 e2 = 0.5f * (hin[4].PosW + hin[24].PosW);
    float3 e3 = 0.5f * (hin[20].PosW + hin[24].PosW);
    
    hconst.TessEdges[0] = CalculateLODTessFactor(e0);
    hconst.TessEdges[1] = CalculateLODTessFactor(e1);
    hconst.TessEdges[2] = CalculateLODTessFactor(e2);
    hconst.TessEdges[3] = CalculateLODTessFactor(e3);
    
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 25;i++)
        sum += hin[i].PosW;
    float3 center = sum / 25.0f;
    hconst.TessInsides[0] = hconst.TessInsides[1] = CalculateLODTessFactor(center);
    
    return hconst;
}

void BernsteinCoeffcient5x5(float t, out float bernstein[5])
{
    float inv_t = 1.0f - t;
    bernstein[0] = inv_t * inv_t * inv_t * inv_t;
    bernstein[1] = 4.0f * t * inv_t * inv_t * inv_t;
    bernstein[2] = 6.0f * t * t * inv_t * inv_t;
    bernstein[3] = 4.0f * t * t * t * inv_t;
    bernstein[4] = t * t * t * t;
}

float3 CubicBezierSum5x5(OutputPatch<HsOut, 25> patch, float uB[5], float vB[5], bool normal)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    
    int i = 0;
    [unroll]
    for (i = 0; i < 5; i++)
    {
        float3 bu = float3(0.0f, 0.0f, 0.0f);
        int j = 0;
        [unroll]
        for (j = 0; j < 5; j++)
        {
            if(normal == false) 
                bu += uB[j] * patch[5 * i + j].PosL;
            else
                bu += uB[j] * patch[5 * i + j].NormalL;
        }
        sum += vB[i] * bu;
    }
    return sum;
}

[domain("quad")]
DsOut DS(HsConstant hconst, float2 uv : SV_DomainLocation, OutputPatch<HsOut, 25> patch)
{
    DsOut dout = (DsOut) 0;
    
    float uB[5], vB[5];
    BernsteinCoeffcient5x5(uv.x, uB);
    BernsteinCoeffcient5x5(uv.y, vB);
    
    float3 pos = CubicBezierSum5x5(patch, uB, vB, false);
    float4 posW = mul(float4(pos, 1.0f), gWorld);
    
    dout.PosH = mul(mul(float4(pos, 1.0f), gWorld), gViewProj);
    float3 normalL = CubicBezierSum5x5(patch, uB, vB, true);
    float4x4 tWorld = transpose(gWorld);
    dout.NormalW = mul((float3x3) tWorld, normalize(normalL));
    dout.TexCoord0 = lerp(lerp(patch[0].TexCoord0, patch[4].TexCoord0, uv.x), lerp(patch[20].TexCoord0, patch[24].TexCoord0, uv.x), uv.y);
    dout.TexCoord1 = lerp(lerp(patch[0].TexCoord1, patch[4].TexCoord1, uv.x), lerp(patch[20].TexCoord1, patch[24].TexCoord1, uv.x), uv.y);
    dout.Tessellation = float4(hconst.TessEdges[0], hconst.TessEdges[1], hconst.TessEdges[2], hconst.TessEdges[3]);
    
    return dout;
}

float4 PS(DsOut din) : SV_Target
{
    float4 result = 0.0f;
    
    if(gKeyInput)
    {
        if (din.Tessellation.w <= 5.0f) 
            result = float4(1.0f, 0.0f, 0.0f, 1.0f);
        else if (din.Tessellation.w <= 10.0f) 
            result = float4(0.0f, 1.0f, 0.0f, 1.0f);
        else if (din.Tessellation.w <= 20.0f)
            result = float4(0.0f, 0.0f, 1.0f, 1.0f);
        else if (din.Tessellation.w <= 30.0f)
            result = float4(1.0f, 1.0f, 0.0f, 1.0f);
        else if (din.Tessellation.w <= 40.0f)
            result = float4(1.0f, 0.0f, 1.0f, 1.0f);
        else if (din.Tessellation.w <= 50.0f)
            result = float4(0.0f, 1.0f, 1.0f, 1.0f);
        else if (din.Tessellation.w <= 60.0f)
            result = float4(1.0f, 1.0f, 1.0f, 1.0f);
        else
            result = float4(1.0f, 0.5f, 0.5f, 1.0f);
    }
    else
    {
        float4 diffuse = 0.0f;
        
        float4 baseTexDiffuse = gBaseTexture.Sample(gAnisotropicWrap, din.TexCoord0) * gMat.Diffuse;
        float4 detailedTexDiffuse = gDetailedTexture.Sample(gAnisotropicWrap, din.TexCoord1) * gMat.Diffuse;
        float4 roadTexDiffuse = gRoadTexture.Sample(gAnisotropicWrap, din.TexCoord0) * gMat.Diffuse;
    
        if (roadTexDiffuse.a < 0.4f)
        {
            diffuse = saturate(baseTexDiffuse * 0.6f + detailedTexDiffuse * 0.4f);
        }
        else
        {
            diffuse = roadTexDiffuse;
        }
        
        float3 view = normalize(gCameraPos - din.PosW);
        float4 ambient = gAmbient * diffuse;
        
        float shadowFactor[3] = { 1.0f, 1.0f, 1.0f };
        for (int i = 0; i < 1;i++)
            shadowFactor[i] = CalcShadowFactor(din.PosS);
        
        Material mat = { diffuse, gMat.Fresnel, gMat.Roughness };
        float4 directLight = ComputeLighting(gLights, mat, normalize(din.NormalW), view, shadowFactor);
    
        result = ambient + directLight;
        result.a = diffuse.a;
    }
    return result;
}