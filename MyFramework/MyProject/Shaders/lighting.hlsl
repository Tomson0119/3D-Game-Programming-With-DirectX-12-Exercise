#define NUM_LIGHTS 1

struct Light
{
    float3 Position;
    float3 Direction;
    float3 Diffuse;
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
    float3 result = float3(0.0f, 0.0f, 0.0f);
    float oneMinusCosine = (1 - dot(h, l));
    
    return fresnel + (1 - fresnel) * Pow5(oneMinusCosine);
}

float3 PhongModelLighting(float3 lightDiff, float3 lightVec, float3 normal, float3 view, Material mat)
{
    const float m = (1.0f - mat.Roughness) * 256.0f;
    
    float3 halfVec = normalize(view + lightVec);
    
    float roughness = (m + 8.0f) / 8.0f * pow(max(dot(halfVec, normal), 0.0f), m);
    float3 fresnel = CalcReflectPercent(mat.Fresnel, halfVec, lightVec);    
    float3 specular = fresnel * roughness;
    
    specular = specular / (specular + 1.0f);
    
    float3 matDiffuse = mat.Diffuse.rgb;
    
    return (matDiffuse + specular) * lightDiff;
}

float3 ComputeDirectLight(Light light, Material mat, float3 normal, float3 view)
{
    float3 lightVec = -light.Direction;
    
    float cosValue = max(dot(normal, lightVec), 0.0f);
    float3 lightDiffuse = light.Diffuse * cosValue;
    
    return PhongModelLighting(lightDiffuse, lightVec, normal, view, mat);
}

float4 ComputeLighting(Light lights[NUM_LIGHTS], Material mat, float3 normal, float3 view)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    
    int i = 0;
    for (i = 0; i < NUM_LIGHTS; ++i)
    {
        result += ComputeDirectLight(lights[i], mat, normal, view);
    }
    
    return float4(result, 1.0f);
}

