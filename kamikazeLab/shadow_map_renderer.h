#ifndef _SHADOW_MAP_RENDERER_H_
#define _SHADOW_MAP_RENDERER_H_

#include "shadow_map.h"
#include "lights.h"
#include "phys_object.h"

struct ShadowMapRenderer
{
	ShadowMapRenderer();

	void render(PhysObject** physObjects, unsigned int count);

	ShadowMap dirLightShadowMaps[DIR_LIGHT_COUNT];
	glm::mat4 dirLightProjectionViews[DIR_LIGHT_COUNT];

private:
	void renderPhysObject(PhysObject& physObject);

	ShadowMapMaterial shadowMapMaterial;
};

#endif