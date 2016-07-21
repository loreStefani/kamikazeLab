layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_textCoord;

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_invProjection;
};

out vec2 v_textCoord;
out vec3 v_viewRay;

void main()
{
	vec4 ndcPos = vec4(a_position, 1.0f);
	
	//place the full screen quad on the z=-1 ndc space plane (near plane)
	ndcPos.z = -1.0f;

	gl_Position = ndcPos;

	v_textCoord = a_textCoord;
	
	vec4 viewRay = u_invProjection * ndcPos;

	viewRay.xyz /= viewRay.w;

	//project the view ray on the z = -1 view space plane (remember that the view space is right handed),
	//so that we can scale it later with the depth to reconstruct the view space position.
	//this operation can be done here since we interpolate on a plane with constant z
	//and there is no risk of perspective "incorrect" interpolation
	v_viewRay = vec3(viewRay.xy / abs(viewRay.z), -1.0f);
}