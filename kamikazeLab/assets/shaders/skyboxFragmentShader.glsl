in vec3 v_worldRay;
in vec2 v_textCoord;

layout(binding = 0) uniform samplerCube u_skyBox;
#ifdef DEFERRED_RENDER
layout(binding = 1) uniform sampler2D u_depthBuffer;
#endif

out vec3 skyBoxColor;

void main()
{	
#ifdef DEFERRED_RENDER
	//depth test
	float depthBufferSample = texture(u_depthBuffer, v_textCoord).r;
	if(depthBufferSample < gl_FragCoord.z)
	{
		discard;
	}
#endif
#ifdef SWAP_SKYBOX_YZ
	skyBoxColor = texture(u_skyBox, v_worldRay.xzy).rgb;
#else
	skyBoxColor = texture(u_skyBox, v_worldRay).rgb;
#endif
}