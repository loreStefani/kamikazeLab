layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_textCoord;
layout(location = 3) in vec4 a_tangentWithHandedness;

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_projection;
};

layout(binding=1, std140) uniform ObjectVertexShaderUniformBlock
{
	mat4 u_viewWorld;	
};

out vec3 v_normal;
out vec2 v_textCoord;
out vec4 v_tangentWithHandedness;

void main()
{
	vec4 pos = vec4(a_position,1.0);
	vec3 norm = a_normal;

	vec4 viewPos = u_viewWorld* pos;

	//assuming uniform scaling here
	//moreover, v_normal it's not normalized here because it will be in the fragment shader anyway
	v_normal = (u_viewWorld* vec4(norm,0.0)).xyz;
	
	vec3 viewTangent = (u_viewWorld* vec4(a_tangentWithHandedness.xyz, 0.0)).xyz;
	v_tangentWithHandedness = vec4(viewTangent, a_tangentWithHandedness.w);

	v_textCoord = a_textCoord;

	gl_Position = u_projection* viewPos;
}