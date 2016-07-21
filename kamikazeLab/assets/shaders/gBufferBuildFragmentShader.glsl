in vec3 v_normal;
in vec2 v_textCoord;
in vec4 v_tangentWithHandedness;

layout (binding = 2, std140) uniform ObjectFragmentShaderUniformBlock
{
	vec4 u_matSpecularAndExponent;
	vec4 u_textCoordScaleAndTranslate;	 
};

layout(binding=0) uniform sampler2D u_diffuseMap;
layout(binding=1) uniform sampler2D u_normalMap;
layout(binding=2) uniform sampler2D u_specMap;


layout(location=0) out vec3 normal;
layout(location=1) out vec3 diffuse;
layout(location=2) out vec4 specularAndExponent;

void main()
{
	vec2 textCoord = v_textCoord*u_textCoordScaleAndTranslate.xy + u_textCoordScaleAndTranslate.zw;

	diffuse = texture(u_diffuseMap,textCoord).rgb;
	
	//normal mapping	
	normal = normalSampleToViewSpace(texture(u_normalMap,textCoord).rgb, normalize(v_normal), v_tangentWithHandedness);
		
	specularAndExponent = u_matSpecularAndExponent;

	//specular mapping
	specularAndExponent.xyz *= texture(u_specMap, textCoord).rgb;
}