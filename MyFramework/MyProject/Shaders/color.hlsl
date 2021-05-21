
cbuffer constants : register(b0)
{
	float4x4 world;
	float4x4 viewProj;
}

struct VertexIn
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITON;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	float4x4 worldViewProj = mul(world, viewProj);
	vout.PosH = mul(float4(vin.PosL, 1.0f), worldViewProj);
	vout.Color = vin.Color;
	
	return vout;
}

float4 PS(VertexOut vout) : SV_Target
{
	return vout.Color;
}