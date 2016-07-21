
vec3 lambertDiffuseBRDF(vec3 diffuseColor)
{
	return diffuseColor;
}

vec3 blinnPhongSpecularBRDF(vec3 normal, vec3 toLight, vec3 toEye, vec3 specularColor, float specularExponent)
{
	vec3 halfWayVector = normalize(toLight + toEye);
	float cosTheta = dot(normal, halfWayVector);
		
	//blinn-phong brdf normalization factor. proof here: http://www.farbrausch.de/~fg/stuff/phong.pdf
	const float normalizationFactorNum = (specularExponent + 2.0f)*(specularExponent + 4.0f);
	
	const float twoToSpecularExponent = pow(2.0f, specularExponent);
	const float twoToSpecularExponentSqrt = sqrt(twoToSpecularExponent);
	const float oneOverTwoToSpecularExponentSqrt = 1.0f / twoToSpecularExponentSqrt;
	float normalizationFactorDen = 8.0f*(oneOverTwoToSpecularExponentSqrt + specularExponent);
	
	float normalizationFactor = normalizationFactorNum / normalizationFactorDen;	

	return specularColor*normalizationFactor*pow(max(cosTheta, 0.0f), specularExponent);
}

vec3 computeReflectedRadiance(vec3 lightRadiance, vec3 normal, vec3 toLight, vec3 toEye, vec3 diffuseColor, vec3 specularColor, float specularExponent)
{
	vec3 incidentIrradiance = lightRadiance*max(dot(normal, toLight), 0.0f);	
	return (lambertDiffuseBRDF(diffuseColor) + blinnPhongSpecularBRDF(normal, toLight, toEye, specularColor, specularExponent))*incidentIrradiance;
}

struct DirectionalLight
{
	vec3 color;
	float pad;
	vec3 direction;
	float pad1;
};

vec3 computeDirLightReflectedRadiance(DirectionalLight l, vec3 normal, vec3 toEye, vec3 diffuseColor, vec3 specularColor, float specularExponent)
{	
	return computeReflectedRadiance(l.color, normal, -l.direction, toEye, diffuseColor, specularColor, specularExponent);
}

struct SpotLight
{
	vec4 attenuationAndSpot;
	vec3 color;
	float pad;
	vec3 direction;
	float pad1;
	vec3 position;
	float pad2;
};

float computeAttenuationFactor(vec3 attenuationCoefficients, float distance)
{
	return min(1.0f / dot(attenuationCoefficients, vec3(1.0f, distance, distance*distance)), 1.0f);
}

vec3 computeSpotLightReflectedRadiance(SpotLight l, vec3 position, vec3 normal, vec3 toEye, vec3 diffuseColor, vec3 specularColor, float specularExponent)
{
	vec3 toLight = l.position - position;
	float distance = length(toLight);

	toLight /= distance;

	vec3 lightRadiance = l.color * pow(max(dot(toLight, l.direction), 0.0f), l.attenuationAndSpot.w);
	float attenuation = computeAttenuationFactor(l.attenuationAndSpot.xyz, distance);
	lightRadiance *= attenuation;

	return computeReflectedRadiance(lightRadiance, normal, toLight, toEye, diffuseColor, specularColor, specularExponent);
}

struct PointLight
{
	vec4 positionAndRadius;
	vec3 color;
	float pad;
	vec3 attenuation;
	float pad1;
};

vec3 computePointLightReflectedRadiance(PointLight l, vec3 position, vec3 normal, vec3 toEye, vec3 diffuseColor, vec3 specularColor, float specularExponent)
{
	vec3 toLight = l.positionAndRadius.xyz - position;
	float distance = length(toLight);

	toLight /= distance;

	vec3 lightRadiance = l.color * computeAttenuationFactor(l.attenuation, distance);
	
	float rangeFactor = 1.0f - float(l.positionAndRadius.w < distance);
	lightRadiance *= rangeFactor;

	return computeReflectedRadiance(lightRadiance, normal, toLight, toEye, diffuseColor, specularColor, specularExponent);
}


//shadow map filtering adapted from : http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping/

float texture2DCompare(sampler2D shadowMap, vec2 uv, float compare)
{
	float depth = texture(shadowMap, uv).r;
	return step(compare, depth);
}

#if defined(SHADOW_LERP) || defined(SHADOW_PCF)

float texture2DShadowLerp(sampler2D shadowMap, vec2 shadowMapSize, vec2 texelSize, vec2 shadowMapCoord, float compare)
{
	vec2 roundTexel = shadowMapCoord*shadowMapSize+0.5f;
	vec2 _centroid = floor(roundTexel)/shadowMapSize;
	float lb = texture2DCompare(shadowMap, _centroid, compare);
	float lt = texture2DCompare(shadowMap, vec2(_centroid.x, _centroid.y + texelSize.y), compare);
	float rb = texture2DCompare(shadowMap, vec2(_centroid.x +texelSize.x, _centroid.y), compare);
	float rt = texture2DCompare(shadowMap, _centroid+texelSize, compare);
	vec2 t = fract(roundTexel);
	float a = mix(lb, lt, t.y);
	float b = mix(rb, rt, t.y);
	float c = mix(a, b, t.x);
	return c;
}
#endif

#ifdef SHADOW_PCF
float PCF(sampler2D shadowMap, vec2 shadowMapSize, vec2 texelSize, vec2 shadowMapCoord, float compare)
{
	float result = 0.0f;
	for(int x=-1; x<=1; x++)
	{
		for(int y=-1; y<=1; y++)
		{
			vec2 off = vec2(x,y)*texelSize;
			result += texture2DShadowLerp(shadowMap, shadowMapSize, texelSize, shadowMapCoord+off, compare);
		}
	}
	return result/9.0f;
}
#endif

float calcShadowFactor(sampler2D shadowMap, float depth, vec2 shadowMapCoord, vec2 shadowMapSize)
{
#ifdef SHADOW_PCF
	vec2 texelSize = vec2(1.0f)/shadowMapSize;
	return PCF(shadowMap, shadowMapSize, texelSize, shadowMapCoord, depth);
#elif defined(SHADOW_LERP)
	vec2 texelSize = vec2(1.0f)/shadowMapSize;
	return texture2DShadowLerp(shadowMap, shadowMapSize, texelSize, shadowMapCoord, depth);
#else
	return texture2DCompare(shadowMap, shadowMapCoord, depth);
#endif
}

