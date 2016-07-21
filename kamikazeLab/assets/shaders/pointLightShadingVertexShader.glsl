layout(location = 0) in vec3 a_position;

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_viewWorld;
	mat4 u_projection;
};

out vec4 v_clipPos;
out vec3 v_viewRay;

void main()
{
	vec4 viewPos = u_viewWorld * vec4(a_position, 1.0f);
	vec4 clipPos = u_projection * viewPos;			

	gl_Position = clipPos;

	v_clipPos = clipPos;
	v_viewRay = viewPos.xyz;
}