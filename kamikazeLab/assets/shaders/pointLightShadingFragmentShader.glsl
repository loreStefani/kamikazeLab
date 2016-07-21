in vec4 v_clipPos;
in vec3 v_viewRay;

layout (binding=1, std140) uniform SceneFragmentShaderUniformBlock
{
	float u_frustumNear;
	float u_frustumFar;
	vec2 scenePad1;
	PointLight u_pointLight;
};

layout(binding=0) uniform sampler2D u_depthBuffer; //float
layout(binding=1) uniform sampler2D u_normalBuffer; //vec3
layout(binding=2) uniform sampler2D u_diffuseBuffer; //vec3
layout(binding=3) uniform sampler2D u_specularAndExponentBuffer; //vec4

out vec4 fragmentRadiance;

void main()
{		
	//complete projection and find projective texture coordinate
	vec3 ndcPos = v_clipPos.xyz / v_clipPos.w;

	vec2 textCoord = (ndcPos.xy + 1.0f)*0.5f;

	float depth = texture(u_depthBuffer, textCoord).r;
	vec3 normal = normalize(texture(u_normalBuffer, textCoord).rgb);
	vec3 diffuse = texture(u_diffuseBuffer, textCoord).rgb;
	vec4 specularAndExponent = texture(u_specularAndExponentBuffer, textCoord);

	vec3 specular = specularAndExponent.xyz;
	float specularExponent = specularAndExponent.w;
	
	float viewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*depth - 1.0f, u_frustumNear, u_frustumFar);

	//project view ray on the z=-1 view space plane
	vec3 viewRay = vec3(v_viewRay.xy / abs(v_viewRay.z), -1.0f);

	vec3 reconstructedPosition = computeViewSpacePositionFromDepth(viewSpaceDepth, viewRay);		
		
	vec3 reconstructedToEye = normalize(-reconstructedPosition);
	vec3 position = reconstructedPosition;
	vec3 toEye = reconstructedToEye;
			
	vec3 reflectedRadiance = 
		computePointLightReflectedRadiance(u_pointLight, position, normal, toEye, diffuse, specular, specularExponent);
	
	fragmentRadiance = vec4(reflectedRadiance, 1.0f);
}