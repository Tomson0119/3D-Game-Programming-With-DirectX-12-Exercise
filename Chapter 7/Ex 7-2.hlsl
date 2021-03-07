//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
cbuffer cbPerObject : register(b0)
{

    // Ex 7-2
    float gWorld0;
    float gWorld1;
    float gWorld2;
    float gWorld3;
    float gWorld4;
    float gWorld5;
    float gWorld6;
    float gWorld7;
    float gWorld8;
    float gWorld9;
    float gWorld10;
    float gWorld11;
    float gWorld12;
    float gWorld13;
    float gWorld14;
    float gWorld15;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    //
    // Ex 7-2
    float4x4 gWorld;
    gWorld._11 = gWorld0;
    gWorld._12 = gWorld1;
    gWorld._13 = gWorld2;
    gWorld._14 = gWorld3;
    gWorld._21 = gWorld4;
    gWorld._22 = gWorld5;
    gWorld._23 = gWorld6;
    gWorld._24 = gWorld7;
    gWorld._31 = gWorld8;
    gWorld._32 = gWorld9;
    gWorld._33 = gWorld10;
    gWorld._34 = gWorld11;
    gWorld._41 = gWorld12;
    gWorld._42 = gWorld13;
    gWorld._43 = gWorld14;
    gWorld._44 = gWorld15;
    //
    //

	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}


