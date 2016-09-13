layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_textCoord;
layout(location = 3) in vec4 a_tangentWithHandedness;

layout(binding=0, std140) uniform SceneVertexShaderUniformBlock
{
	mat4 u_projView;	
	mat4 u_dirShadowTransforms[DIR_LIGHT_COUNT];
};

layout(binding=1, std140) uniform ObjectVertexShaderUniformBlock
{
	mat4 u_world;	
};

out vec3 v_position;
out vec3 v_normal;
out vec2 v_textCoord;
out vec4 v_tangentWithHandedness;

out vec4 v_dirShadowPosition[DIR_LIGHT_COUNT];

void main()
{
	vec4 pos = vec4(a_position,1.0);
	vec3 norm = a_normal;

	vec4 worldPos = u_world* pos;

	v_position = worldPos.xyz;
	
	//assuming uniform scaling here
	//moreover, v_normal it's not normalized here because it will be in the fragment shader anyway
	v_normal = (u_world* vec4(norm,0.0)).xyz;
	
	vec3 worldTangent = (u_world* vec4(a_tangentWithHandedness.xyz, 0.0)).xyz;
	v_tangentWithHandedness = vec4(worldTangent, a_tangentWithHandedness.w);

	v_textCoord = a_textCoord;

	for(int i = 0 ; i < DIR_LIGHT_COUNT; i++)
	{
		v_dirShadowPosition[i] = u_dirShadowTransforms[i] * worldPos;
		v_dirShadowPosition[i].xyz *= 0.5f;
		v_dirShadowPosition[i].xyz += 0.5f;
	}

	gl_Position = u_projView* worldPos;
}