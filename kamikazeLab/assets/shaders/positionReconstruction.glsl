float computeViewDepthFromNDCDepthInternal(float ndcDepth, float projection22, float projection23)
{
	//proof here: http://www.derschmale.com/2014/01/26/reconstructing-positions-from-the-depth-buffer/	
	return -projection23 / (ndcDepth + projection22);
}

float computeViewDepthFromNDCDepth(float ndcDepth, float frustumNear, float frustumFar)
{	
	const float farMinusNear = frustumFar - frustumNear;
	const float p22 = -(frustumNear + frustumFar) / farMinusNear;
	const float p23 = (-2.0f*frustumNear*frustumFar) / farMinusNear;
	
	return computeViewDepthFromNDCDepthInternal(ndcDepth, p22, p23);
}

float computeViewDepthFromNDCDepth(float ndcDepth, mat4 projection)
{
	return computeViewDepthFromNDCDepthInternal(ndcDepth, projection[2][2], projection[3][2]); //glsl matrices are accessed column-major
}

//assuming viewRay lies on the z=-1 view space plane
vec3 computeViewSpacePositionFromDepth(float depth, vec3 viewRay)
{	
	return abs(depth)*viewRay;
	
	//another technique could be the following
	//float t = depth / viewRay.z;
	//return vec3(viewRay.xy*t, depth);	
}
