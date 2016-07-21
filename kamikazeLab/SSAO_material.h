#ifndef _SSSAO_MATERIAL_H_
#define _SSSAO_MATERIAL_H_

#include <glm/glm.hpp>

#define SSAO_SAMPLES_COUNT 14 //beware that the current implementation relies on this value.
#define SSAO_OCCLUSION_RADIUS 0.5f
#define SSAO_EPSILON 0.01f
#define SSAO_MIN_DISTANCE 0.05f
#define SSAO_MAX_DISTANCE 1.4f

struct SSAOMaterial
{
	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_invProjection;
	};

	struct SceneFragmentShaderUniformBlock
	{
		glm::mat4 u_projection;
		float u_frustumNear;
		float u_frustumFar;
		glm::vec2 u_randomDirectionTiling;
	};

	struct ConstantFragmentShaderUniformBlock
	{
		glm::vec4 u_sampleOffsets[SSAO_SAMPLES_COUNT];
	};

	SSAOMaterial();

	static void bind();
	static GpuProgram gpuProgram;

	static void updateSceneData();

	static SceneVertexShaderUniformBlock sceneVertexShaderUniformBlock;
	static SceneFragmentShaderUniformBlock sceneFragmentShaderUniformBlock;
	static ConstantFragmentShaderUniformBlock constantFragmentShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;
	static unsigned int sceneFragmentShaderUniformBufferId;
	static unsigned int constantFragmentShaderUniformBufferId;
		
	static GpuTexture depthBuffer;
	static GpuTexture normalBuffer;
	static GpuTexture randomDirectionsTexture;

	static constexpr unsigned int kRandomDirectionTextureSize = 1024;
};


#endif
