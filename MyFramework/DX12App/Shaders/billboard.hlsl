#include "common.hlsl"

#define PARTICLE_TYPE_EMMITER 0
#define PARTICLE_TYPE_FLARE 1

Texture2DArray gTexture : register(t0);

struct VertexIn
{
	float3 PosL      : POSITION;
    float2 Size      : SIZE;
    float3 Direction : DIRECTION;
    float2 Age       : LIFETIME;
    float  Speed     : SPEED;
    uint   Type      : TYPE;
};

struct GeoOut
{
    float4 PosH     : SV_POSITION;
    uint   PrimID   : SV_PrimitiveID;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
    float2 TexCoord : TEXCOORD;
    float2 Age      : LIFETIME;
    uint   Type     : TYPE;
};

VertexIn VSStreamOutput(VertexIn vin)
{
    return (vin);
}

[maxvertexcount(2)]
void GSStreamOutput(point VertexIn gin[1],
                    inout PointStream<VertexIn> pointStream)
{
    VertexIn particle = gin[0];
    
    particle.Age.x += gElapsedTime;
    
    if (particle.Age.x <= particle.Age.y)
    {
        if (particle.Type == PARTICLE_TYPE_EMMITER)
        {
            particle.Age.x = 0.0f;
            particle.PosL = gPlayerPos;
            pointStream.Append(particle);
            
            VertexIn vertex = (VertexIn)0;            
            vertex.PosL = particle.PosL;
            vertex.Size = particle.Size;
            vertex.Direction = gRandFloat4.xyz;
            vertex.Speed = particle.Speed * gRandFloat4.w;               
            vertex.Type = PARTICLE_TYPE_FLARE;
            vertex.Age.x = 0.0f;
            vertex.Age.y = 10.0f;
            pointStream.Append(vertex);
        }
        else
        {
            particle.PosL += particle.Direction * gElapsedTime * particle.Speed;
            pointStream.Append(particle);
        }        
    }
}

VertexIn VSRender(VertexIn vin)
{
    return (vin);
}

[maxvertexcount(4)]
void GSRender(point VertexIn gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream)
{
    float3 posW = mul(float4(gin[0].PosL, 1.0f), gWorld).xyz;
    
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gCameraPos - posW;
    
    look.y = 0.0f;
    look = normalize(look);
    
    float3 right = cross(up, look);
    
    float hw = gin[0].Size.x * 0.5f;
    float hh = gin[0].Size.y * 0.5f;
    
    float4 v[4];
    v[0] = float4(posW + hw * right - hh * up, 1.0f);
    v[1] = float4(posW + hw * right + hh * up, 1.0f);
    v[2] = float4(posW - hw * right - hh * up, 1.0f);
    v[3] = float4(posW - hw * right + hh * up, 1.0f);
    
    float2 TexCoord[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4;i++)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.TexCoord = TexCoord[i];
        gout.PrimID = primID;
        gout.Type = gin[0].Type;
        gout.Age = gin[0].Age;
        
        triStream.Append(gout);
    }
}

float4 PSRender(GeoOut pin) : SV_Target
{
    float3 uvw = float3(pin.TexCoord, pin.PrimID % 4);
    float4 diffuse = gTexture.Sample(gAnisotropicWrap, uvw) * gMat.Diffuse;
    
    clip(diffuse.a - 0.1f);
    
    return diffuse;
}