
//convert the normal sample from tangent space to the space in which normal and tangentWithHandedness lie
vec3 transformNormalSample(vec3 normalMapSample, vec3 normal, vec4 tangentWithHandedness)
{
	//convert from [0,1] to [-1,1]
	normalMapSample = normalize(2.0 * normalMapSample - vec3(1.0,1.0,1.0));

	vec3 tangent = tangentWithHandedness.xyz;
	float handedness = tangentWithHandedness.w;

	tangent = normalize( tangent - dot(tangent,normal)*normal);
	vec3 bitangent = cross(normal,tangent) * sign(handedness);
	
	mat3 TBN = mat3(tangent, bitangent, normal);
	return TBN*normalMapSample;
}

vec3 normalSampleToWorldSpace(vec3 normalMapSample, vec3 worldNormal, vec4 worldTangentWithHandedness)
{
	return transformNormalSample(normalMapSample, worldNormal, worldTangentWithHandedness);
}

vec3 normalSampleToViewSpace(vec3 normalMapSample, vec3 viewNormal, vec4 viewTangentWithHandedness)
{
	return transformNormalSample(normalMapSample, viewNormal, viewTangentWithHandedness);
}
