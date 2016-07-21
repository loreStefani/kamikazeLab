#ifndef _SKYBOX_MATERIAL_H_
#define _SKYBOX_MATERIAL_H_

#include <glm/glm.hpp>
#include "shader.h"
#include "texture_cube.h"
#include "render_path.h"

//#define SKYBOX_SWAP_SKYBOX_YZ

struct SkyBoxMaterial
{
	struct SceneVertexShaderUniformBlock
	{
		glm::mat4 u_invProjection;
		glm::mat4 u_invView;
	};

	SkyBoxMaterial();

	static void bind();
	static GpuProgram gpuProgram;

	static void updateSceneData();

	void bindInstance()const;

	static SceneVertexShaderUniformBlock sceneVertexShaderUniformBlock;

	static unsigned int sceneVertexShaderUniformBufferId;

	GpuTextureCube skyBox;

#ifndef FORWARD_RENDER
	//if the DeferredRender is used, the depth buffer attached to the frame buffer we draw the skybox into is empty.
	//we then perform the depth test manually, sampling the GBuffer depth component
	static GpuTexture depthBuffer;
#endif
};


#endif
