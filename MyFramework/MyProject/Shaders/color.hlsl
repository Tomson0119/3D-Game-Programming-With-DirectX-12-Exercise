
cbuffer CommonCB : register(b0)
{
    matrix gView : packoffset(c0);
    matrix gProj : packoffset(c4);
    matrix gViewProj : packoffset(c8);
}

cbuffer ObjectCB : register(b1)
{
    matrix gWorld : packoffset(c0);
}

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
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosH = mul(posW, gViewProj);
	vout.Color = vin.Color;
	
	return vout;
}

float4 PS(VertexOut vout) : SV_Target
{
	return vout.Color;
}