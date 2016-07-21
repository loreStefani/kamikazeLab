#ifndef _EDGE_PRESERVING_BLUR_MATERIAL_H_
#define _EDGE_PRESERVING_BLUR_MATERIAL_H_

#define BLUR_KERNEL_RADIUS 5

struct EdgePreservingBlurMaterial
{
	struct SceneFragmentShaderUniformBlock
	{
		glm::vec2 u_texelSize;
		float u_frustumNear;
		float u_frustumFar;
	};

	struct ConstantFragmentShaderUniformBlock
	{
		float u_weights[BLUR_KERNEL_RADIUS*2 + 1];		
	};

	EdgePreservingBlurMaterial();

	static void bindHorizontal();
	static void bindVertical();

	static GpuProgram horizontalGpuProgram;
	static GpuProgram verticalGpuProgram;

	static void setTexelSize(glm::vec2 texelSize);
	static void updateSceneData();

	static SceneFragmentShaderUniformBlock sceneFragmentShaderUniformBlock;
	static ConstantFragmentShaderUniformBlock constantFragmentShaderUniformBlock;

	static unsigned int sceneFragmentShaderUniformBufferId;
	static unsigned int constantFragmentShaderUniformBufferId;

	static GpuTexture depthBuffer;
	static GpuTexture normalBuffer;
	static GpuTexture colorMap;

private:
	static void bind();
};

#endif
