#define NUM_LIGHTS 3

#define POINT_LIGHT       1
#define SPOT_LIGHT        2
#define DIRECTIONAL_LIGHT 3

struct Light
{
    float3 Diffuse;
    float  padding0;
    float3 Position;
    float  padding1;
    float3 Direction;
    float  padding2;
    float  Range;
    int    Type;
};

struct Material
{
    float4 Diffuse;
    float3 Fresnel;
    float Roughness;
};

float Pow5(float x)
{
    return (x * x * x * x * x);
}

float3 CalcReflectPercent(float3 fresnel, float3 h, float3 l)
{
    float oneMinusCosine = 1.0f - saturate(dot(h, l));
    return (fresnel + (1.0f - fresnel) * Pow5(oneMinusCosine));
}

float3 PhongModelLighting(float3 lightDiff, float3 lightVec, float3 normal, float3 view, Material mat)
{
    const float shininess = 1.0f - mat.Roughness;
    const float m = shininess * 256.0f;
    
    float3 halfVec = normalize(view + lightVec);
    
    float roughness = (m + 8.0f) / 8.0f * pow(saturate(dot(halfVec, normal)), m);
    float3 fresnel = CalcReflectPercent(mat.Fresnel, halfVec, lightVec);    
    float3 specular = fresnel * roughness;
    
    float3 matDiffuse = mat.Diffuse.rgb;
    
    return (matDiffuse + specular) * lightDiff;
}

float3 ComputeDirectLight(Light light, Material mat, float3 normal, float3 view)
{
    float3 lightVec = normalize(light.Direction);
    
    float cosValue = max(dot(normal, lightVec), 0.0f);
    float3 lightDiffuse = light.Diffuse * cosValue;
    
    return PhongModelLighting(lightDiffuse, lightVec, normal, view, mat);
}

float4 ComputeLighting(Light lights[NUM_LIGHTS], Material mat, 
                       float3 normal, float3 view, float shadowFactor[NUM_LIGHTS])
{
    float3 result = 0.0f;
    
    int i = 0;
    
    [unroll]
    for (i = 0; i < NUM_LIGHTS; ++i)
    {        
        if (lights[i].Type == DIRECTIONAL_LIGHT)
            result += shadowFactor[i] * ComputeDirectLight(lights[i], mat, normal, view);
        else if(lights[i].Type == SPOT_LIGHT)
            ;
        else
            ;
    }
    
    return float4(result, 0.0f);
}

