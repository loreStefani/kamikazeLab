#ifndef _SSAO_RENDERER_H_
#define _SSAO_RENDERER_H_

#include "SSAO_map.h"
#include "SSAO_material.h"
#include "mesh.h"
#include "edge_preserving_blur_material.h"

struct SSAORenderer 
{
	SSAORenderer();
	
	void render()const;

	SSAOMap ssaoMap;

private:
	GpuMesh fullScreenQuad;
	SSAOMaterial ssaoMaterial;
	EdgePreservingBlurMaterial blurMaterial;
	SSAOMap blurIntermediateBuffer;

};


#endif
