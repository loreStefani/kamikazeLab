#ifndef _SKYBOX_RENDERER_H_
#define _SKYBOX_RENDERER_H_

#include "skybox_material.h"


struct SkyBoxRenderer
{
	SkyBoxRenderer();

	void render()const;
	SkyBoxMaterial skyBoxMaterial;

private:
	GpuMesh fullScreenQuad;
};

#endif
