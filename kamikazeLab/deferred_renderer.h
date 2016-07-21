#ifndef _DEFERRED_RENDERER_H_
#define _DEFERRED_RENDERER_H_

#include "gbuffer.h"
#include "mesh.h"
#include "deferred_materials.h"

class PhysObject;
struct SSAORenderer;

struct DeferredRenderer
{
	DeferredRenderer();
	~DeferredRenderer();

	GBuffer gbuffer;
		
	void render(PhysObject** physObjects, unsigned int count)const;

	std::unique_ptr<SSAORenderer> ssaoRenderer;

private:
	void renderPhysObject(PhysObject& physObject)const;

	GpuMesh fullScreenQuad;
	GpuMesh sphere;

	DirLightDeferredShadingMaterial dirLightDeferredMaterial;
	PointLightDeferredShadingMaterial pointLightDeferredMaterial;	
};
#endif