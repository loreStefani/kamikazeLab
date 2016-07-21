#ifndef _DEFERRED_MATERIAL_H_
#define  _DEFERRED_MATERIAL_H_

#include <glm/glm.hpp>
#include "shader.h"
#include "texture.h"
#include "lights.h"
#include "graphics_resource.h"

struct DeferredMaterial
{
	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_projection;
	};

	struct ObjectVertexShaderUniformBlock
	{
		glm::mat4 u_viewWorld;
	};

	struct ObjectFragmentShaderUniformBlock
	{
		glm::vec4 u_matSpecularAndExponent;
		glm::vec4 u_textCoordScaleAndTranslate;
	};
	
	DeferredMaterial();

	static void bind();
	static GpuProgram gpuProgram;

	void setWorldTransform(const glm::mat4& world);
	void setSpecularColor(const glm::vec3& specularColor);
	void setSpecularExponent(float specularExponent);
	void setTextCoordScale(const glm::vec2& textCoordScale);
	void setTextCoordTranslate(const glm::vec2& textCoordTranslate);

	void bindInstance()const;

	void updateUniforms();
	static void updateSceneData();

	static SceneVertexShaderUniformBlock sceneVertexShaderUniformBlock;

	ObjectVertexShaderUniformBlock objectVertexShaderUniformBlock;
	ObjectFragmentShaderUniformBlock objectFragmentShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;

	unsigned int objectVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
	unsigned int objectFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

	GpuTexture diffuseMap;
	GpuTexture normalMap;
	GpuTexture specularMap;
};

struct DirLightDeferredShadingMaterial
{
	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_invProjection;
	};

	struct SceneFragmentShaderUniformBlock
	{
		glm::vec3 u_ambientLight;
		float scenePad; 
		float u_frustumNear;
		float u_frustumFar;
		glm::vec2 scenePad1;
		DirectionalLight u_directionalLights[DIR_LIGHT_COUNT];
		glm::vec4 u_dirShadowMapSizeAndBias[DIR_LIGHT_COUNT];
		glm::mat4 u_dirShadowTransforms[DIR_LIGHT_COUNT];
	};
	
	DirLightDeferredShadingMaterial();

	static void bind();
	static GpuProgram gpuProgram;

	static void updateSceneData();

	static SceneVertexShaderUniformBlock sceneVertexShaderUniformBlock;
	static SceneFragmentShaderUniformBlock sceneFragmentShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;
	static unsigned int sceneFragmentShaderUniformBufferId;

	static GpuTexture dirShadowMaps[DIR_LIGHT_COUNT];

	static GpuTexture ssaoMap;

	static GpuTexture depthBuffer;
	static GpuTexture normalBuffer;
	static GpuTexture diffuseBuffer;
	static GpuTexture specularAndExponentBuffer;
};

struct PointLightDeferredShadingMaterial
{
	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_viewWorld;
		glm::mat4 u_projection;
	};

	struct SceneFragmentShaderUniformBlock
	{
		float u_frustumNear;
		float u_frustumFar;
		glm::vec2 scenePad1;
		PointLight u_pointLight;
	};

	PointLightDeferredShadingMaterial();

	static void bind();
	static GpuProgram gpuProgram;

	static void updateLightData(unsigned int pointLightIndex);
	static void updateSceneData();

	static SceneVertexShaderUniformBlock sceneVertexShaderUniformBlock;
	static SceneFragmentShaderUniformBlock sceneFragmentShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;
	static unsigned int sceneFragmentShaderUniformBufferId;

	static GpuTexture depthBuffer;
	static GpuTexture normalBuffer;
	static GpuTexture diffuseBuffer;
	static GpuTexture specularAndExponentBuffer;
};

#endif