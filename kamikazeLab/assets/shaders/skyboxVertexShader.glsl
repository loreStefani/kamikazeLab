layout(location = 0) in vec3 a_position;
#ifdef DEFERRED_RENDER
layout(location = 2) in vec2 a_textCoord;
#endif

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_invProjection;
	mat4 u_invView;
};

out vec3 v_worldRay;
#ifdef DEFERRED_RENDER
out vec2 v_textCoord;
#endif

void main()
{
	vec4 ndcPos = vec4(a_position, 1.0f);
	
	//place the full screen quad on the z=1 ndc space plane (far plane), required for depth testing
	ndcPos.z = 1.0f;

	gl_Position = ndcPos;

	vec4 viewRay = u_invProjection * ndcPos;
	viewRay.xyz /= viewRay.w;

	v_worldRay = mat3(u_invView) * viewRay.xyz;

#ifdef DEFERRED_RENDER
	v_textCoord = a_textCoord;
#endif
}