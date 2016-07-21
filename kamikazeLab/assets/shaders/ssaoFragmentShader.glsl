in vec3 v_viewRay;
in vec2 v_textCoord;

layout (binding=1, std140) uniform SceneFragmentShaderUniformBlock
{
	mat4 u_projection;	
	float u_frustumNear;
	float u_frustumFar;
	vec2 u_randomDirectionTiling;
};

layout (binding=2, std140) uniform ConstantFragmentShaderUniformBlock
{
	vec4 u_sampleOffsets[SAMPLES_COUNT];
};


layout(binding = 0) uniform sampler2D u_depthBuffer;
layout(binding = 1) uniform sampler2D u_normalBuffer;
layout(binding = 2) uniform sampler2D u_randomDirections;

out float accessibility;
                                                                                                            
void main()
{
	float depth = texture(u_depthBuffer, v_textCoord).r;
	vec3 normal = normalize(texture(u_normalBuffer, v_textCoord).rgb);
	
	float viewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*depth - 1.0f, u_frustumNear, u_frustumFar);
	vec3 viewPosition = computeViewSpacePositionFromDepth(viewSpaceDepth, v_viewRay);          
	                                           
    //get random offset
    vec3 randomOffset = 2.0f * texture(u_randomDirections, v_textCoord * u_randomDirectionTiling).xyz - 1.0f;
		
    float occlusionSum = 0.0f;
                                                                                
    for(int i = 0; i < SAMPLES_COUNT; i++)
	{
        //get random direction, flip it if behind the plane defined by the surface point and the normal
        vec3 dir = reflect(u_sampleOffsets[i].xyz, randomOffset);
        float flip = sign(dot(dir, normal));
        //get random point on the hemisphere
        vec3 q = viewPosition + flip*dir*OCCLUSION_RADIUS;

        //get projective texture coordinate
        vec4 projectedQ = u_projection*vec4(q, 1.0f);
        vec2 qPTC = (projectedQ.xy/projectedQ.w + 1.0f) * 0.5f;

        //get depth of the nearest(wrt the camera) point r along the ray going from the camera through q
		float rViewDepth = texture(u_depthBuffer, qPTC).r;

        //reconstruct view position        
		float rViewSpaceDepth = computeViewDepthFromNDCDepth(2.0f*rViewDepth - 1.0f, u_frustumNear, u_frustumFar);
		vec3 qViewRay = vec3(q.xy / abs(q.z), -1.0f);
		vec3 r = computeViewSpacePositionFromDepth(rViewSpaceDepth, qViewRay);     

        float frontPlaneWeight = max(dot(normal, normalize(r-viewPosition)), 0.0f);
        float dz = -(viewPosition.z - r.z);
                                                                                        
        float occlusion = 0.0f;
                                                                                        
        if(dz > EPSILON)
		{
            occlusion = frontPlaneWeight* clamp((MAX_DISTANCE - dz)/float(MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
		}

        occlusionSum += occlusion;
    }

    accessibility = pow(1.0 - occlusionSum/float(SAMPLES_COUNT), 8.0f);	
}