in vec3 v_position;
in vec3 v_normal;
in vec2 v_textCoord;
in vec4 v_tangentWithHandedness;

in vec4 v_dirShadowPosition[DIR_LIGHT_COUNT];

layout (binding = 3, std140) uniform ObjectFragmentShaderUniformBlock
{
	vec4 u_matSpecularAndExponent;
	vec4 u_textCoordScaleAndTranslate;	 
};

layout(binding=0) uniform sampler2D u_colorMap;
layout(binding=1) uniform sampler2D u_normalMap;
layout(binding=2) uniform sampler2D u_specMap;

layout (binding=2, std140) uniform SceneFragmentShaderUniformBlock
{
	vec3 u_ambientLight;
	float scenePad;
	vec3 u_eyePosition;
	float scenePad1;
	DirectionalLight u_directionalLights[DIR_LIGHT_COUNT];
	PointLight u_pointLights[POINT_LIGHT_COUNT];
	vec4 u_dirShadowMapSizeAndBias[DIR_LIGHT_COUNT];
};

layout(binding= 4)uniform sampler2D u_dirShadowMaps[DIR_LIGHT_COUNT];

out vec3 fragmentRadiance;

void main()
{
	vec3 normal = normalize(v_normal);
	vec3 toEye = normalize(u_eyePosition- v_position);

	vec2 textCoord = v_textCoord*u_textCoordScaleAndTranslate.xy + u_textCoordScaleAndTranslate.zw;

	vec3 diffuseColor = texture(u_colorMap,textCoord).rgb;
	vec3 specularColor = u_matSpecularAndExponent.xyz;
	float specularExponent = u_matSpecularAndExponent.w;

	specularColor *= texture(u_specMap, textCoord).rgb;

	//normal mapping	
	normal = normalSampleToWorldSpace(texture(u_normalMap,textCoord).rgb, normal, v_tangentWithHandedness);
		
	vec3 reflectedRadiance = vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < DIR_LIGHT_COUNT; i++)
	{
		vec3 dirLightReflectedRadiance = computeDirLightReflectedRadiance(u_directionalLights[i], normal, toEye, diffuseColor, specularColor, specularExponent);
		vec4 shadowPos = v_dirShadowPosition[i];
		float depth = shadowPos.z;
		vec2 shadowMapCoord = shadowPos.xy;
		vec3 shadowMapParams = u_dirShadowMapSizeAndBias[i].xyz;
		float shadowFactor = calcShadowFactor(u_dirShadowMaps[i], depth + shadowMapParams.z, shadowMapCoord, shadowMapParams.xy);
		bvec4 inside = bvec4(shadowMapCoord.x >= 0.0, shadowMapCoord.x <= 1.0, shadowMapCoord.y >= 0.0, shadowMapCoord.y <= 1.0);
		bool infrustum = all(inside);
		infrustum = all(bvec2(infrustum, depth <= 1.0));
		float occlusion = float(infrustum)*(shadowFactor);
		reflectedRadiance += occlusion * dirLightReflectedRadiance;
	}
	
	for (int i = 0 ; i < POINT_LIGHT_COUNT;i++)
	{
		reflectedRadiance += 
			computePointLightReflectedRadiance(u_pointLights[i], v_position, normal, toEye, diffuseColor, specularColor, specularExponent);		
	}

	vec3 ambientTerm = u_ambientLight*diffuseColor;
	fragmentRadiance = ambientTerm + reflectedRadiance;
}