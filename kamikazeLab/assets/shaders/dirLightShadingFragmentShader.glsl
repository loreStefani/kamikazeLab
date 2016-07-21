in vec2 v_textCoord;
in vec3 v_viewRay;

layout (binding=1, std140) uniform SceneFragmentShaderUniformBlock
{
	vec3 u_ambientLight;
	float scenePad; 
	float u_frustumNear;
	float u_frustumFar;
	vec2 scenePad1;	
	DirectionalLight u_directionalLights[DIR_LIGHT_COUNT];
	vec4 u_dirShadowMapSizeAndBias[DIR_LIGHT_COUNT];
	mat4 u_dirShadowTransforms[DIR_LIGHT_COUNT]; //view space (main camera space) -> light ndc space
};

layout(binding= 4) uniform sampler2D u_dirShadowMaps[DIR_LIGHT_COUNT];

layout(binding=0) uniform sampler2D u_depthBuffer; //float
layout(binding=1) uniform sampler2D u_normalBuffer; //vec3
layout(binding=2) uniform sampler2D u_diffuseBuffer; //vec3
layout(binding=3) uniform sampler2D u_specularAndExponentBuffer; //vec4

layout(binding=4+DIR_LIGHT_COUNT) uniform sampler2D u_ssaoMap;

out vec3 fragmentRadiance;

void main()
{			
	float depth = texture(u_depthBuffer, v_textCoord).r;
	vec3 normal = normalize(texture(u_normalBuffer, v_textCoord).rgb);
	vec3 diffuse = texture(u_diffuseBuffer, v_textCoord).rgb;
	vec4 specularAndExponent = texture(u_specularAndExponentBuffer, v_textCoord);

	vec3 specular = specularAndExponent.xyz;
	float specularExponent = specularAndExponent.w;
	
	float viewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*depth - 1.0f, u_frustumNear, u_frustumFar);
	vec3 reconstructedPosition = computeViewSpacePositionFromDepth(viewSpaceDepth, v_viewRay);		
		
	vec3 reconstructedToEye = normalize(-reconstructedPosition);
	vec3 position = reconstructedPosition;
	vec3 toEye = reconstructedToEye;

	vec3 reflectedRadiance = vec3(0.0f, 0.0f, 0.0f);
	
	for (int i = 0; i < DIR_LIGHT_COUNT; i++)
	{
		vec3 dirLightReflectedRadiance = computeDirLightReflectedRadiance(u_directionalLights[i], normal, toEye, diffuse, specular, specularExponent);
		vec4 shadowPos = u_dirShadowTransforms[i] * vec4(position, 1.0f); //no need to divide by w as the projection transform is orthographic
		shadowPos.xyz *= 0.5f;
		shadowPos.xyz += 0.5f;
		float depth = shadowPos.z;
		vec2 shadowMapCoord = shadowPos.xy;				
		vec3 shadowMapParams = u_dirShadowMapSizeAndBias[i].xyz;
		float shadowFactor = calcShadowFactor(u_dirShadowMaps[i], depth + shadowMapParams.z, shadowMapCoord, shadowMapParams.xy);
		bvec4 inside = bvec4(shadowMapCoord.x >= 0.0, shadowMapCoord.x <= 1.0, shadowMapCoord.y >= 0.0, shadowMapCoord.y <= 1.0);
		bool infrustum = all(inside);
		infrustum = all(bvec2(infrustum, depth <= 1.0));
		float occlusion = float(infrustum)*shadowFactor;
		reflectedRadiance += dirLightReflectedRadiance * occlusion;		
	}
	
	vec3 ambientTerm = u_ambientLight*diffuse;
	float accessibility = texture(u_ssaoMap, v_textCoord).r;
	ambientTerm *= accessibility;
	
	fragmentRadiance = ambientTerm + reflectedRadiance;
}