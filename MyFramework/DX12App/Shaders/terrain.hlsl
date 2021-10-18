#include "lighting.hlsl"

Texture2D gBaseTexture     : register(t0);
Texture2D gDetailedTexture : register(t1);
Texture2D gRoadTexture     : register(t2);
Texture2D gHeightmap       : register(t3);
Texture2D gNormalmap       : register(t4);

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

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

VertexIn VS(VertexIn vin)
{
    return vin;
}

VertexIn midPoint(VertexIn a, VertexIn b)
{
    VertexIn mid;
    
    mid.PosL = (a.PosL + b.PosL) * 0.5f;
    mid.NormalL = (a.NormalL + b.NormalL) * 0.5f;
    mid.TexCoord0 = (a.TexCoord0 + b.TexCoord0) * 0.5f;
    mid.TexCoord1 = (a.TexCoord1 + b.TexCoord1) * 0.5f;
    
    return mid;
}

[maxvertexcount(8)]
void GS(triangle VertexIn gin[3], inout TriangleStream<GeoOut> triStream)
{
    //      v0
    //     /  \
    //   m0 -- m1
    //   / \  / \
    // v1 - m2 - v2
    VertexIn vertices[6];
    vertices[0] = gin[0];
    vertices[1] = gin[1];
    vertices[2] = gin[2];
    vertices[3] = midPoint(gin[0], gin[1]);
    vertices[4] = midPoint(gin[1], gin[2]);
    vertices[5] = midPoint(gin[0], gin[2]);
    
    float3 edge1 = gin[1].PosL - gin[0].PosL;
    float3 edge2 = gin[2].PosL - gin[0].PosL;
    float2 duv1 = gin[1].TexCoord0 - gin[0].TexCoord0;
    float2 duv2 = gin[2].TexCoord0 - gin[0].TexCoord0;
    
    float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);
    
    float3 tangent;
    tangent.x = f * (duv2.y * edge1.x - duv1.y * edge2.x);
    tangent.y = f * (duv2.y * edge1.y - duv1.y * edge2.y);
    tangent.z = f * (duv2.y * edge1.z - duv1.y * edge2.z);
    
    GeoOut gout[6];
    
    [unroll]
    for (int i = 0; i < 6;i++)
    {
        gout[i].PosW = mul(float4(vertices[i].PosL, 1.0f), gWorld).xyz;        
        gout[i].PosH = mul(float4(gout[i].PosW, 1.0f), gViewProj);
        float4x4 tWorld = transpose(gWorld);
        gout[i].NormalW = mul((float3x3)tWorld, vertices[i].NormalL);
        gout[i].TangentW = mul((float3x3)tWorld, tangent);
        gout[i].TexCoord0 = vertices[i].TexCoord0;
        gout[i].TexCoord1 = vertices[i].TexCoord1;
    }
    triStream.Append(gout[0]);
    triStream.Append(gout[3]);
    triStream.Append(gout[5]);
    triStream.Append(gout[4]);
    triStream.Append(gout[2]);
    triStream.RestartStrip();
    triStream.Append(gout[3]);
    triStream.Append(gout[1]);
    triStream.Append(gout[4]);
    triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
    float4 baseTexDiffuse = gBaseTexture.Sample(gSamplerState, pin.TexCoord0) * gMat.Diffuse;
    float4 detailedTexDiffuse = gDetailedTexture.Sample(gSamplerState, pin.TexCoord1) * gMat.Diffuse;
    float4 roadTexDiffuse = gRoadTexture.Sample(gSamplerState, pin.TexCoord0) * gMat.Diffuse;
    
    float4 finalDiffuse = 0.0f;
    if (roadTexDiffuse.a < 0.4f)
    {
        finalDiffuse = saturate(baseTexDiffuse * 0.6f + detailedTexDiffuse * 0.4f);
    }
    else
    {
        finalDiffuse = roadTexDiffuse;
    }
    
    pin.NormalW = normalize(pin.NormalW);
    
    float4 normalMapColor = gNormalmap.Sample(gSamplerState, pin.TexCoord0);
    float3 normalVec = 2.0f * normalMapColor.rgb - 1.0f;
    
    float3 N = pin.NormalW;
    float3 T = normalize(pin.TangentW - dot(pin.TangentW, N) * N);
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    float3 normalW = mul(normalVec, TBN);
    
    float3 view = normalize(gCameraPos - pin.PosW);
    float4 ambient = gAmbient * finalDiffuse;
    
    Material mat = { finalDiffuse, gMat.Fresnel, gMat.Roughness };
    float4 directLight = ComputeLighting(gLights, mat, normalW, view);
    
    float4 result = finalDiffuse + directLight;
    result.a = gMat.Diffuse.a;
    
    return result;
}