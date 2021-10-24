#include "lighting.hlsl"

SamplerState gAnisotropicWrap  : register(s0);
SamplerState gAnisotropicClamp : register(s1);
SamplerState gLinearWrap       : register(s2);
SamplerState gLinearClamp      : register(s3);

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