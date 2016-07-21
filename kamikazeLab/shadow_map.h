#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include <glm\glm.hpp>
#include "shader.h"
#include "texture.h"
#include "graphics_resource.h"

struct ShadowMap
{
	void init(unsigned int width, unsigned int height);
	
	void resize(unsigned int width, unsigned int height);
	
	void bindAsDepthBuffer()const;
	void bindAsSampler()const;
	
	void unbind()const;

	void release();

	unsigned int frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;
	
	GpuTexture depthTexture;
	unsigned int width = 0;
	unsigned int height = 0;
	float bias = 0.0f;
};

struct ShadowMapMaterial
{
	struct LightVertexShaderUniformBlock
	{
		glm::mat4 u_projView;
	};

	struct ObjectVertexShaderUniformBlock
	{
		glm::mat4 u_world;
	};

	ShadowMapMaterial();

	void setWorldTransform(const glm::mat4& world);
	void bindInstance()const;

	void updateObjectUniforms();
	void updateLightUniforms();
	void setLightProjectionView(const glm::mat4& lightProjectionView);
	const glm::mat4& getLightProjectionView()const;

	static GpuProgram gpuProgram;

	LightVertexShaderUniformBlock lightVertexShaderUniformBlock;
	ObjectVertexShaderUniformBlock objectVertexShaderUniformBlock;

	unsigned int lightVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
	unsigned int objectVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
};

#endif