#define MaxLights 16

struct Light
{
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
};

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	// Linear falloff.
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));

	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal,
	float3 toEye, Material mat)
{
	const float m = mat.Shininess * 256.0f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = fresnelFactor * roughnessFactor;

	// Our spec formula goes outside [0,1] range, but we are
	// doing LDR rendering. So scale it down a bit.
	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

	float3 diffuse = mat.DiffuseAlbedo.rgb;

	/*float diffuseLength = sqrt(diffuse.x * diffuse.x +
		diffuse.y * diffuse.y + diffuse.z * diffuse.z);
	float specLength = sqrt(specAlbedo.x * specAlbedo.x +
		specAlbedo.y * specAlbedo.y + specAlbedo.z * specAlbedo.z);*/

	/*if (diffuseLength <= 0.5f)
	{
		diffuse.x = 0.0f;
		diffuse.y = 0.0f;
		diffuse.z = 0.0f;
	}
	else if (diffuseLength <= 0.8f)
	{
		diffuse.x = 0.4f;
		diffuse.y = 0.4f;
		diffuse.z = 0.4f;
	}
	else
	{
		diffuse.x = 1.0f;
		diffuse.y = 1.0f;
		diffuse.z = 1.0f;
	}

	if (specLength <= 0.0f)
	{
		specAlbedo.x = 0.1f;
		specAlbedo.y = 0.1f;
		specAlbedo.z = 0.1f;
	}
	else if (specLength <= 0.4f)
	{
		specAlbedo.x = 0.3f;
		specAlbedo.y = 0.3f;
		specAlbedo.z = 0.3f;
	}
	else
	{
		specAlbedo.x = 0.8f;
		specAlbedo.y = 0.8f;
		specAlbedo.z = 0.8f;
	}*/

	return (diffuse + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float toEye)
{
	// The light vector aims opposite the direction the light rays travel.
	float3 lightVec = -L.Direction;

	// Scale light down by Lambert's cosine law.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float toEye)
{
	// The vector from the surface to the light.
	float3 lightVec = L.Position - pos;

	// The distance from surface to light.
	float d = length(lightVec);

	// Range test.
	if (d > L.FalloffEnd)
		return 0.0f;

	// Normalize the light vector.
	lightVec /= d;

	// Scale light down by Lambert's cosine law.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	// Attenuate light by distance.
	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	// The vector from the surface to the light.
	float3 lightVec = L.Position - pos;

	// The distance from surface to light.
	float d = length(lightVec);

	// Range test.
	if (d > L.FalloffEnd)
		return 0.0f;

	// Normalize the light vector
	lightVec /= d;

	// Scale light down by Lambert's cosine law.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	// Attenuate light by distance.
	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	// Scale by spotlight.
	float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
	lightStrength *= spotFactor;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
						float3 pos, float3 normal, float3 toEye,
						float3 shadowFactor)
{
	float3 result = 0.0f;

	int i = 0;

#if (NUM_DIR_LIGHTS > 0)
	for (i = 0; i < NUM_DIR_LIGHTS; ++i)
	{
		result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
	}
#endif

#if (NUM_POINT_LIGHTS > 0)
	for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
	{
		result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
	}
#endif

#if (NUM_SPOT_LIGHTS > 0)
	for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
	{
		result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
	}
#endif

	return float4(result, 0.0f);
}