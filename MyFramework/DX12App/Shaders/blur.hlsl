Texture2D gScreen0 : register(t0);
Texture2D gScreen1 : register(t1);
RWTexture2D<float4> gOutput : register(u0);

groupshared float4 gCache[32 + 2][32 + 2];


void AddTextureIntoCache(int3 groupThreadID : SV_GroupThreadID,
                         int3 dispatchThreadID : SV_DispatchThreadID)
{
    const float ratio = 0.8f;
    
    gCache[groupThreadID.x + 1][groupThreadID.y + 1]
        = lerp(gScreen0[dispatchThreadID.xy], gScreen1[dispatchThreadID.xy], ratio);
    
    if (groupThreadID.x == 0 && groupThreadID.y == 0)
        gCache[0][0] = lerp(gScreen0[int2(dispatchThreadID.x - 1, dispatchThreadID.y - 1)],
                            gScreen1[int2(dispatchThreadID.x - 1, dispatchThreadID.y - 1)],
                            ratio);
    if (groupThreadID.x == 31 && groupThreadID.y == 0)
        gCache[33][0] = lerp(gScreen0[int2(dispatchThreadID.x + 1, dispatchThreadID.y - 1)],
                             gScreen1[int2(dispatchThreadID.x + 1, dispatchThreadID.y - 1)],
                             ratio);
    if (groupThreadID.x == 0 && groupThreadID.y == 31)
        gCache[0][33] = lerp(gScreen0[int2(dispatchThreadID.x - 1, dispatchThreadID.y + 1)],
                             gScreen1[int2(dispatchThreadID.x - 1, dispatchThreadID.y + 1)],
                             ratio);
    if (groupThreadID.x == 31 && groupThreadID.y == 31)
        gCache[33][33] = lerp(gScreen0[int2(dispatchThreadID.x + 1, dispatchThreadID.y + 1)],
                              gScreen1[int2(dispatchThreadID.x + 1, dispatchThreadID.y + 1)],
                              ratio);
    
    if (groupThreadID.x == 0)
        gCache[0][groupThreadID.y + 1] = lerp(gScreen0[int2(dispatchThreadID.x - 1, dispatchThreadID.y)],
                                              gScreen1[int2(dispatchThreadID.x - 1, dispatchThreadID.y)],
                                              ratio);
    if (groupThreadID.y == 0)
        gCache[groupThreadID.x + 1][0] = lerp(gScreen0[int2(dispatchThreadID.x, dispatchThreadID.y - 1)],
                                              gScreen1[int2(dispatchThreadID.x, dispatchThreadID.y - 1)],
                                              ratio);
    if (groupThreadID.x == 31)
        gCache[33][groupThreadID.y + 1] = lerp(gScreen0[int2(dispatchThreadID.x + 1, dispatchThreadID.y)],
                                               gScreen1[int2(dispatchThreadID.x + 1, dispatchThreadID.y)],
                                               ratio);
    if (groupThreadID.y == 31)
        gCache[groupThreadID.x + 1][33] = lerp(gScreen0[int2(dispatchThreadID.x, dispatchThreadID.y + 1)],
                                               gScreen1[int2(dispatchThreadID.x, dispatchThreadID.y + 1)],
                                               ratio);
}

[numthreads(32, 32, 1)]
void CS(int3 groupThreadID : SV_GroupThreadID,
        int3 dispatchThreadID : SV_DispatchThreadID)
{
    AddTextureIntoCache(groupThreadID, dispatchThreadID);    
    
    GroupMemoryBarrierWithGroupSync();
    
    float3 color = float3(0, 0, 0);
    int i = 0;
    int j = 0;
    [unroll]
    for (i = -1; i <= 1; i++)
    {
        [unroll]
        for (j = -1; j <= 1; j++)
        {
            color += gCache[groupThreadID.x + 1 + j][groupThreadID.y + 1 + i].rgb * 0.1f;
        }
    }
    
    gOutput[dispatchThreadID.xy] = float4(color, 1.0f);
}