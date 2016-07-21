layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_textCoord;

uniform mat4 u_projectionView;
uniform mat4 u_world;

void main()
{
	gl_Position = u_projectionView * u_world * vec4(a_position, 1.0f);
}