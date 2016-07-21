in vec2 v_textCoord;

layout(binding=0) uniform sampler2D u_depthBuffer;
layout(binding=1) uniform sampler2D u_normalBuffer; 
layout(binding=2) uniform sampler2D u_colorMap; 

layout (binding=0, std140) uniform SceneFragmentShaderUniformBlock
{	
	vec2 u_texelSize;		
	float u_frustumNear;
	float u_frustumFar;
};

vec3 processSample(vec2 sampleTextCoord, float weight, inout float weightAccumulator, vec3 referenceNormal, float referenceViewSpaceDepth)
{
	float sampleDepth = texture(u_depthBuffer, sampleTextCoord).r;
	vec3 sampleNormal = normalize(texture(u_normalBuffer, sampleTextCoord).rgb);
	float sampleViewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*sampleDepth - 1.0f, u_frustumNear, u_frustumFar);	

	if(dot(sampleNormal, referenceNormal) >= 0.8f && abs(sampleViewSpaceDepth - referenceViewSpaceDepth) <= 0.2f)
	{
		weightAccumulator += weight;
		return weight * texture(u_colorMap, sampleTextCoord).rgb;		
	}

	return vec3(0.0f, 0.0f, 0.0f);
}

out float blurredFragment;

void main()
{
	float depth = texture(u_depthBuffer, v_textCoord).r;
	vec3 normal = normalize(texture(u_normalBuffer, v_textCoord).rgb);
	float viewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*depth - 1.0f, u_frustumNear, u_frustumFar);	
		
	#ifdef HORIZONTAL
	vec2 texelOffset = vec2(u_texelSize.x, 0.0f);
	#else
	vec2 texelOffset = vec2(0.0f, u_texelSize.y);
	#endif

	vec3 result = weights[KERNEL_RADIUS] * texture(u_colorMap, v_textCoord).rgb;
	float totalWeight = weights[KERNEL_RADIUS];

	//weights is a globally declared (elsewhere) float array of size KERNEL_RADIUS*2+1

	for(int i = -KERNEL_RADIUS; i < 0; ++i)
	{		
		vec2 sampleTextCoord = v_textCoord + i*texelOffset;		
		result += processSample(sampleTextCoord, weights[i + KERNEL_RADIUS], totalWeight, normal, viewSpaceDepth);
	}

	for(int i = 1; i <= KERNEL_RADIUS; ++i)
	{
		vec2 sampleTextCoord = v_textCoord + i*texelOffset;
		result += processSample(sampleTextCoord, weights[i + KERNEL_RADIUS], totalWeight, normal, viewSpaceDepth);
	}

	blurredFragment = result.x / totalWeight;
}