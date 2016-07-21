#ifndef _GBUFFER_H_
#define _GBUFFER_H_

#include "texture.h"
#include "graphics_resource.h"

struct GBuffer
{
	void init(unsigned int width, unsigned int height);

	void resize(unsigned int width, unsigned int height);

	void bind()const;	
	void unbind()const;

	void release();

	unsigned int frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;

	GpuTexture depthTexture;
	GpuTexture normalTexture;
	GpuTexture diffuseTexture;
	GpuTexture specularAndExponentTexture;

	unsigned int width = 0;
	unsigned int height = 0;	
};
#endif