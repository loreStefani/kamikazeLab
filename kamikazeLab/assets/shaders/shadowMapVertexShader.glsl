layout(location = 0) in vec3 a_position;

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_projView;	
};

layout(binding=1, std140) uniform ObjectVertexShaderUniformBlock
{
	mat4 u_world;	
};

void main()
{
	gl_Position = u_projView * u_world * vec4(a_position, 1.0f);
}