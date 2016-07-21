#ifndef _FORWARD_MATERIAL_H_
#define _FORWARD_MATERIAL_H_

#include "shader.h"
#include <glm/glm.hpp>
#include "texture.h"
#include "lights.h"
#include "graphics_resource.h"

struct ForwardMaterial
{
	struct ObjectVertexShaderUniformBlock
	{
		glm::mat4 u_world;
	};

	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_projView;
		glm::mat4 u_dirShadowTransforms[DIR_LIGHT_COUNT];
	};
	
	struct ObjectFragmentShaderUniformBlock
	{
		glm::vec4 u_matSpecularAndExponent;
		glm::vec4 u_textCoordScaleAndTranslate;
	};

	struct SceneFragmentShaderUniformBlock
	{
		glm::vec3 u_ambientLight;
		float scenePad;
		glm::vec3 u_eyePosition;
		float scenePad1;
		DirectionalLight u_directionalLights[DIR_LIGHT_COUNT];
		PointLight u_pointLights[POINT_LIGHT_COUNT];
		glm::vec4 u_dirShadowMapSizeAndBias[DIR_LIGHT_COUNT];
	};

	ForwardMaterial();

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
	static SceneFragmentShaderUniformBlock sceneFragmentShaderUniformBlock;

	ObjectVertexShaderUniformBlock objectVertexShaderUniformBlock;
	ObjectFragmentShaderUniformBlock objectFragmentShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;
	static unsigned int sceneFragmentShaderUniformBufferId;

	unsigned int objectVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
	unsigned int objectFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

	GpuTexture diffuseMap;
	GpuTexture normalMap;
	GpuTexture specularMap;

	static GpuTexture dirShadowMaps[DIR_LIGHT_COUNT];	
};


#endif