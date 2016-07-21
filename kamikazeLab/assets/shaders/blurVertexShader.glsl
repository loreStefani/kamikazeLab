layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_textCoord;

out vec2 v_textCoord;

void main()
{
	gl_Position = vec4(a_position, 1.0f);
	
	v_textCoord = a_textCoord;	
}