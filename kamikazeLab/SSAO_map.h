#ifndef _SSAO_MAP_H_
#define _SSAO_MAP_H_

#include "texture.h"

struct SSAOMap
{
	void init(unsigned int width, unsigned int height);

	void resize(unsigned int width, unsigned int height);

	void bind()const;
	void unbind()const;

	void release();

	unsigned int frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;
	unsigned int renderBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;

	GpuTexture ssaoTexture;

	unsigned int width = 0;
	unsigned int height = 0;
};


#endif
