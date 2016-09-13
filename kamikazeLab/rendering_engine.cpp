/* rendering engine
 *
 * all methods of all classes, and all global functions, which deal with graphics:
 * rendering, uploading stuff to GPU, etc
 *
 */

#include<Windows.h>

 // We use OpenGL (with glew) but the rest of the code is fairly independent from this.
 // It should be trivial to switch to, e.g., directX

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/random.hpp>
#include "transform.h"
#include "phys_object.h"
#include "custom_classes.h"
#include "mesh.h"
#include "texture.h"
#include "window.h"
#include "forward_material.h"
#include "forward_renderer.h"
#include "deferred_renderer.h"
#include "shadow_map_renderer.h"
#include "assets.h"
#include "SSAO_material.h"
#include "SSAO_renderer.h"
#include "SSAO_map.h"
#include "edge_preserving_blur_material.h"
#include "texture_cube.h"
#include "skybox_material.h"
#include "skybox_renderer.h"
#include "render_path.h"

static void clearOpenGLErrors()
{
	glGetError();
}

static void checkOpenGLErrors()
{
	GLenum error;
	if ((error = glGetError()) != GL_NO_ERROR)
	{
		assert(false);
	}
}

#define OPENGL_CALL(x) clearOpenGLErrors(); (x); checkOpenGLErrors()

/*		CpuMesh		*/

GpuMesh CpuMesh::uploadToGPU()const
{
	GpuMesh res;

	OPENGL_CALL(glGenVertexArrays(1, &res.vertexArrayId));
	OPENGL_CALL(glBindVertexArray(res.vertexArrayId));

	//vertices

	OPENGL_CALL(glCreateBuffers(1, &res.geomBufferId));
	
	OPENGL_CALL(glNamedBufferData(
		res.geomBufferId, // a buffer containing vertices
		sizeof(Vertex)*verts.size(), // how many bytes to copy on GPU
		&(verts[0]), // location in CPU of the data to copy on GPU
		GL_STATIC_DRAW // please know that this buffer is readonly!
	));

	OPENGL_CALL(glBindVertexBuffer(0, res.geomBufferId, 0, sizeof(Vertex)));
		
	OPENGL_CALL(glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos)));
	OPENGL_CALL(glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, norm)));
	OPENGL_CALL(glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv)));
	OPENGL_CALL(glVertexAttribFormat(3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tang)));

	//here, we assume that every vertex shader will have:
	//the position attribute at index 0
	//the normal attribute at index 1
	//the uv attribute at index 2
	OPENGL_CALL(glVertexAttribBinding(0, 0)); //position
	OPENGL_CALL(glVertexAttribBinding(1, 0)); //normal
	OPENGL_CALL(glVertexAttribBinding(2, 0)); //uv
	OPENGL_CALL(glVertexAttribBinding(3, 0)); //tangent with handedness

	OPENGL_CALL(glEnableVertexAttribArray(0));
	OPENGL_CALL(glEnableVertexAttribArray(1));
	OPENGL_CALL(glEnableVertexAttribArray(2));
	OPENGL_CALL(glEnableVertexAttribArray(3));

	//elements

	OPENGL_CALL(glCreateBuffers(1, &res.connBufferId));

	OPENGL_CALL(glNamedBufferData(
		res.connBufferId, // a buffer containing vertex indices
		sizeof(Tri)*tris.size(),
		&(tris[0]),
		GL_STATIC_DRAW
	));

	res.nElements = tris.size() * 3;

	OPENGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, res.geomBufferId));
	OPENGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, res.connBufferId));

	OPENGL_CALL(glBindVertexArray(0));

	OPENGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
	OPENGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	return res;
}

/*		GpuMesh		*/

void GpuMesh::bind()const
{
	OPENGL_CALL(glBindVertexArray(vertexArrayId));
}

void GpuMesh::render() const
{
	OPENGL_CALL(glDrawElements(GL_TRIANGLES, nElements, GL_UNSIGNED_INT, 0));
}

void GpuMesh::release()
{
	if (vertexArrayId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glBindVertexArray(vertexArrayId));
		OPENGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
		OPENGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		OPENGL_CALL(glBindVertexArray(0));
		OPENGL_CALL(glDeleteVertexArrays(1, &vertexArrayId));
	}

	if (geomBufferId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteBuffers(1, &geomBufferId));
	}

	if (connBufferId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteBuffers(1, &connBufferId));
	}	

	vertexArrayId = INVALID_GRAPHICS_RESOURCE_ID;
	geomBufferId = INVALID_GRAPHICS_RESOURCE_ID;
	connBufferId = INVALID_GRAPHICS_RESOURCE_ID;
}

/*		CpuTexture		*/

GpuTexture CpuTexture::uploadToGPU() const
{
	GpuTexture res;
	OPENGL_CALL(glGenTextures(1, &res.textureId));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, res.textureId));

	GLint internalFormat = isLinear ? GL_RGB : GL_SRGB;

	OPENGL_CALL(glTexImage2D(
		GL_TEXTURE_2D,
		0, // higest res mipmap level
		internalFormat,
		sizeX, sizeY,
		0,
		GL_RGBA, // becasue our class Texel is made like this
		GL_UNSIGNED_BYTE, // idem
		&(data[0])
	));

	if (generateMipMaps)
	{
		OPENGL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
	}

	// let's determine how this texture will be accessed (by the fragment shader)!
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	if (generateMipMaps)
	{
		OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	}
	else
	{
		OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	}
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

	return res;
}


/*		GpuTexture		*/

void GpuTexture::bind() const
{
	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
}

void GpuTexture::release()
{
	if (textureId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteTextures(1, &textureId));
		textureId = INVALID_GRAPHICS_RESOURCE_ID;
	}
}

/*		CpuTextureCube		*/

GpuTextureCube CpuTextureCube::uploadToGPU() const
{
	GpuTextureCube res;
	OPENGL_CALL(glGenTextures(1, &res.textureId));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, res.textureId));

	GLint internalFormat = isLinear ? GL_RGB : GL_SRGB;
	
	constexpr size_t facesCount = 6;
	for (size_t i = 0; i < facesCount; ++i)
	{
		OPENGL_CALL(glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			internalFormat,
			sizeX, sizeY,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			&(facesData[i][0])
		));		
	}

	if (generateMipMaps)
	{
		OPENGL_CALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
	}

	OPENGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	if (generateMipMaps)
	{
		OPENGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	}
	else
	{
		OPENGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	}
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

	return res;
}


/*		CpuProgram		*/

static void deleteShader(GLuint shaderId)
{
	OPENGL_CALL(glDeleteShader(shaderId));
}

static bool uploadShaderToGPU(GLenum shaderType, const std::vector<std::string>& shaderSources, GLuint& shaderId)
{
	OPENGL_CALL((shaderId = glCreateShader(shaderType)));

	const size_t sourcesCount = shaderSources.size();

	std::vector<const GLchar*> sources(sourcesCount + 1);
	std::vector<GLint> sourcesSizes(sourcesCount + 1);

	sources[0] = "#version 450\n";
	sourcesSizes[0] = std::strlen(sources[0]);

	for (size_t i = 1; i < sourcesCount + 1; ++i)
	{
		sources[i] = shaderSources[i - 1].c_str();
		sourcesSizes[i] = shaderSources[i - 1].size();
	}

	OPENGL_CALL(glShaderSource(shaderId, sourcesCount + 1, sources.data(), sourcesSizes.data()));

	OPENGL_CALL(glCompileShader(shaderId));

	GLint compilationSucceeded = GL_FALSE;
	OPENGL_CALL(glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compilationSucceeded));

	if (compilationSucceeded == GL_FALSE)
	{
		GLint infoLogSize = 0;
		OPENGL_CALL(glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogSize));

		char* infoLog = new char[infoLogSize];
		OPENGL_CALL(glGetShaderInfoLog(shaderId, infoLogSize, nullptr, infoLog));

		OutputDebugString(infoLog);

		delete[] infoLog;

		deleteShader(shaderId);
		shaderId = 0;

		return false;
	}

	return true;
}

GpuProgram CpuProgram::uploadToGPU() const
{
	GpuProgram gpuProgram;

	GLuint vertexShaderId;
	GLuint fragmentShaderId;

	if (!uploadShaderToGPU(GL_VERTEX_SHADER, vertexShaderSources, vertexShaderId))
	{
		return gpuProgram;
	}

	if (!uploadShaderToGPU(GL_FRAGMENT_SHADER, fragmentShaderSources, fragmentShaderId))
	{
		deleteShader(vertexShaderId);
		return gpuProgram;
	}

	gpuProgram.programId = glCreateProgram();

	OPENGL_CALL(glAttachShader(gpuProgram.programId, vertexShaderId));
	OPENGL_CALL(glAttachShader(gpuProgram.programId, fragmentShaderId));

	OPENGL_CALL(glLinkProgram(gpuProgram.programId));

	GLint linkingSucceeded = GL_FALSE;
	OPENGL_CALL(glGetProgramiv(gpuProgram.programId, GL_LINK_STATUS, &linkingSucceeded));

	OPENGL_CALL(glDetachShader(gpuProgram.programId, vertexShaderId));
	OPENGL_CALL(glDetachShader(gpuProgram.programId, fragmentShaderId));

	deleteShader(vertexShaderId);
	deleteShader(fragmentShaderId);

	if (linkingSucceeded != GL_TRUE)
	{
		GLint infoLogSize = 0;
		OPENGL_CALL(glGetProgramiv(gpuProgram.programId, GL_INFO_LOG_LENGTH, &infoLogSize));

		char* infoLog = new char[infoLogSize];
		OPENGL_CALL(glGetProgramInfoLog(gpuProgram.programId, infoLogSize, nullptr, infoLog));

		OutputDebugString(infoLog);

		delete[] infoLog;

		OPENGL_CALL(glDeleteProgram(gpuProgram.programId));
		gpuProgram.programId = INVALID_GRAPHICS_RESOURCE_ID;
	}

	return gpuProgram;
}

/*		GpuProgram		*/

void GpuProgram::bind()const
{
	assert(programId != INVALID_GRAPHICS_RESOURCE_ID);
	OPENGL_CALL(glUseProgram(programId));
}

void GpuProgram::release()
{
	if (programId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		//TODO: be sure program.programId is not the currently used program
		OPENGL_CALL(glDeleteProgram(programId));
		programId = INVALID_GRAPHICS_RESOURCE_ID;
	}
}

/*		Transform		*/

void Transform::setModelMatrix(glm::mat4& modelMatrix) const
{	
	glm::mat4 rotMat = mat4_cast(ori);
	glm::mat4 traMat(1.0f);
	glm::mat4 scaMat;

	scaMat[0][0] = scale;
	scaMat[1][1] = scale;
	scaMat[2][2] = scale;

	traMat[3] = vec4(pos, 1);

	modelMatrix = traMat * rotMat * scaMat;	
}

glm::mat4 Transform::getModelMatrix()const
{
	glm::mat4 m;
	setModelMatrix(m);
	return m;
}

/*		PhysObject		*/

void PhysObject::setAccumulatedTransform(glm::mat4& accumulatedTransform)const
{
	t.setModelMatrix(accumulatedTransform);

	glm::mat4 meshComponentModelMatrix;
	meshComponent.t.setModelMatrix(meshComponentModelMatrix);

	accumulatedTransform *= meshComponentModelMatrix;
}

glm::mat4 PhysObject::getAccumulatedTransform()const
{
	glm::mat4 accumulatedTransform;
	setAccumulatedTransform(accumulatedTransform);
	return accumulatedTransform;
}

void PhysObject::setCameraInside(glm::mat4& out)const
{
	glm::vec3 up{ 0.0f, 0.0f, 1.0f };
	glm::vec3 cameraPos = t.pos + up*1.2f;
	out = glm::lookAt(cameraPos, t.pos + t.forward()*4.0f, up);
}

/*		Camera		*/

void Camera::computeViewFromTransform()
{
	viewTransform = mat4();
	transform.inverse().setModelMatrix(viewTransform);
}

void Camera::computeInvView()
{
	invViewTransform = glm::inverse(viewTransform);
}

void Camera::computeInvProj()
{
	invProjectionTransform = glm::inverse(projectionTransform);
}

void Camera::computeProjectionView()
{
	projectionViewTransform = projectionTransform * viewTransform;
}

void Camera::setProjectionParams(float _fovY, float _aspectRatio, float _nearPlane, float _farPlane)
{
	nearPlane = _nearPlane;
	farPlane = _farPlane;
	fovY = _fovY;
	aspectRatio = _aspectRatio;

	projectionTransform = glm::perspectiveRH(fovY, aspectRatio, nearPlane, farPlane);
}

/*		Scene		*/

/* metodo globale che disegna la scena */
void rendering()
{
	OPENGL_CALL(glViewport(0, 0, windowWidth, windowHeight));

	OPENGL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

	OPENGL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	scene.render();
}

/* metodo globale che inizializza il sistema grafico */
void initRendering()
{
	glewInit();

	//"OpenGL 4.5 is required"
	assert(GLEW_VERSION_4_5);

	OPENGL_CALL(glEnable(GL_DEPTH_TEST));
	OPENGL_CALL(glDepthFunc(GL_LEQUAL));

	OPENGL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));
}


glm::mat4 Scene::cameraOnTwoObjects(const PhysObject& a, const PhysObject& b)
{
	vec3 center = (a.t.pos + b.t.pos)*0.5f;
	float radius = length(a.t.pos - b.t.pos) / 2.0f + 2.0f;
	
	return glm::translate(glm::vec3{ 0.0f, 0.0f, -2.0f }) * glm::scale(glm::vec3{ 1.0f / radius, 1.0f / radius, 1.0f / radius })
		* glm::translate(glm::vec3{ -center.x, -center.y, -center.z });
}

void Scene::render()
{
	glm::vec3 center = (ships[0].t.pos + ships[1].t.pos)*0.5f;
	float radius = length(ships[0].t.pos - ships[1].t.pos) / 2.0f + 2.0f;

	const float cameraHalfFovY = camera.fovY * 0.5f;
	float cameraZ = radius / tan(cameraHalfFovY);
	
	camera.transform.pos = glm::vec3{ center.x, center.y, cameraZ };
	camera.computeViewFromTransform();

	//TODO
	//ships[0].setCameraInside(camera.viewTransform);

	camera.computeInvView();
	camera.computeProjectionView();

	for (int i = 0; i < 2; ++i)
	{
		glm::vec4 shipPos = ships[i].getAccumulatedTransform()* glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		lighting->pointLights[i].positionAndRadius = glm::vec4{ shipPos.x, shipPos.y, shipPos.z, 8.0f };
	}

	findVisiblePhysObjects();	

	shadowMapRenderer->render(physObjects.data(), physObjects.size());
	
#ifdef FORWARD_RENDER
	forwardRenderer->render(physObjects.data(), physObjects.size());
#else
	deferredRenderer->render(physObjects.data(), physObjects.size());
#endif

	skyBoxRenderer->render();
}

void Scene::findVisiblePhysObjects()
{
	physObjects.clear();

	for (auto& s : ships)
	{
		physObjects.push_back(&s);
		for (auto& b : s.bullets)
		{
			if (b.alive)
			{
				physObjects.push_back(&b);
			}
		}
	}

	physObjects.push_back(floor.get());
}


/*		DeferredRenderer		*/

static GpuMesh getFullScreenQuad()
{
	if (!g_meshLibrary.exists("FullScreenQuad"))
	{
		CpuMesh fullScreenQuad;
		fullScreenQuad.buildFullScreenQuad();
		g_meshLibrary.add("FullScreenQuad", fullScreenQuad);
	}

	return g_meshLibrary.get("FullScreenQuad");
}

DeferredRenderer::DeferredRenderer()
{
	gbuffer.init(windowWidth, windowHeight);
			
	if (!g_meshLibrary.exists("PointLightMesh"))
	{
		CpuMesh pointLightMesh;
		pointLightMesh.buildSphere(1.0f, 20, 20);
		g_meshLibrary.add("PointLightMesh", pointLightMesh);
	}
	
	fullScreenQuad = getFullScreenQuad();
	sphere = g_meshLibrary.get("PointLightMesh");

	ssaoRenderer.reset(new SSAORenderer{});
}

DeferredRenderer::~DeferredRenderer() = default;

void DeferredRenderer::render(PhysObject** physObjects, unsigned int count)const
{
	DeferredMaterial::updateSceneData();

	DeferredMaterial::bind();

	gbuffer.bind();

	for (unsigned int i = 0; i < count; ++i)
	{
		renderPhysObject(*physObjects[i]);
	}

	gbuffer.unbind();

	ssaoRenderer->render();

	OPENGL_CALL(glViewport(0, 0, windowWidth, windowHeight));

	OPENGL_CALL(glDisable(GL_DEPTH_TEST));

	DirLightDeferredShadingMaterial::updateSceneData();
	DirLightDeferredShadingMaterial::bind();

	fullScreenQuad.bind();
	fullScreenQuad.render();

	PointLightDeferredShadingMaterial::updateSceneData();
	PointLightDeferredShadingMaterial::bind();

	sphere.bind();

	OPENGL_CALL(glEnable(GL_BLEND));
	OPENGL_CALL(glBlendEquation(GL_FUNC_ADD));
	OPENGL_CALL(glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO)); //sum rgb, replace alpha

	for (int i = 0; i < POINT_LIGHT_COUNT; ++i)
	{
		PointLightDeferredShadingMaterial::updateLightData(i);
		sphere.render();
	}

	OPENGL_CALL(glDisable(GL_BLEND));
	OPENGL_CALL(glEnable(GL_DEPTH_TEST));
}

static void renderPhysObject(PhysObject& physObject)
{
	glm::mat4 modelMatrix = physObject.getAccumulatedTransform();

	physObject.meshComponent.material.setWorldTransform(modelMatrix);

	physObject.meshComponent.material.updateUniforms();

	physObject.meshComponent.material.bindInstance();

	physObject.meshComponent.mesh.bind();
	physObject.meshComponent.mesh.render();
}

void DeferredRenderer::renderPhysObject(PhysObject& physObject)const
{
	::renderPhysObject(physObject);
}



/*		GBuffer		*/

static void initGBufferComponentParameters()
{
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

void GBuffer::init(unsigned int width, unsigned int height)
{
	OPENGL_CALL(glGenFramebuffers(1, &frameBufferObjectId));
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));

	unsigned int texturesIds[] = { INVALID_GRAPHICS_RESOURCE_ID,
		INVALID_GRAPHICS_RESOURCE_ID,
		INVALID_GRAPHICS_RESOURCE_ID,
		INVALID_GRAPHICS_RESOURCE_ID,
		INVALID_GRAPHICS_RESOURCE_ID };

	OPENGL_CALL(glCreateTextures(GL_TEXTURE_2D, 5, texturesIds));

	depthTexture.textureId = texturesIds[0];
	normalTexture.textureId = texturesIds[1];
	diffuseTexture.textureId = texturesIds[2];
	specularAndExponentTexture.textureId = texturesIds[3];

	depthTexture.bind();
	initGBufferComponentParameters();

	normalTexture.bind();
	initGBufferComponentParameters();

	diffuseTexture.bind();
	initGBufferComponentParameters();

	specularAndExponentTexture.bind();
	initGBufferComponentParameters();

	resize(width, height);

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));//TODO: fetch current texture object id and rebind it here

	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture.textureId, 0));
	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, normalTexture.textureId, 0));
	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, diffuseTexture.textureId, 0));
	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, specularAndExponentTexture.textureId, 0));

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	GLenum colorBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	OPENGL_CALL(glDrawBuffers(4, colorBuffers));

	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0)); //TODO: fetch current frame buffer object id and rebind it here
}

void GBuffer::resize(unsigned int _width, unsigned int _height)
{
	if (_width == width && _height == height)
	{
		return;
	}

	assert(depthTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);
	assert(normalTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);
	assert(diffuseTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);
	assert(specularAndExponentTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);

	depthTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

	normalTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, nullptr));

	diffuseTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _width, _height, 0, GL_RGB, GL_FLOAT, nullptr));

	specularAndExponentTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_FLOAT, nullptr));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0)); //TODO: fetch current texture id and rebind it here

	width = _width;
	height = _height;
}

void GBuffer::bind()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));

	OPENGL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
	OPENGL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	OPENGL_CALL(glViewport(0, 0, width, height));
}

void GBuffer::unbind()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void GBuffer::release()
{
	if (frameBufferObjectId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteFramebuffers(1, &frameBufferObjectId));
		frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;
	}

	depthTexture.release();
	normalTexture.release();
	diffuseTexture.release();
	specularAndExponentTexture.release();
}




/*		ForwardRenderer		*/

void ForwardRenderer::render(PhysObject** physObjects, unsigned int count)const
{
	ForwardMaterial::updateSceneData();

	ForwardMaterial::bind();

	OPENGL_CALL(glViewport(0, 0, windowWidth, windowHeight));

	for (unsigned int i = 0; i < count; ++i)
	{
		renderPhysObject(*physObjects[i]);
	}
}

void ForwardRenderer::renderPhysObject(PhysObject& physObject)const
{
	::renderPhysObject(physObject);
}




/*		ShadowMapRenderer		*/

ShadowMapRenderer::ShadowMapRenderer()
{
	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		dirLightShadowMaps[i].init(2048, 2048);
		dirLightShadowMaps[i].bias = -0.005f;
	}	
}

void ShadowMapRenderer::render(PhysObject** physObjects, unsigned int count)
{
	const float arenaRadius = scene.arenaRadius;

	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		dirLightShadowMaps[i].bindAsDepthBuffer();

		glm::vec3 pseudoDirLightPos = -scene.lighting->directionalLights[i].direction*arenaRadius;

		dirLightProjectionViews[i] = glm::ortho(-arenaRadius*1.2f, arenaRadius*1.2f, -arenaRadius*1.2f, arenaRadius*1.2f, 0.01f, 2 * arenaRadius);
		dirLightProjectionViews[i] *= glm::lookAtRH(pseudoDirLightPos, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });

		shadowMapMaterial.setLightProjectionView(dirLightProjectionViews[i]);
		shadowMapMaterial.updateLightUniforms();

		for (unsigned int i = 0; i < count; ++i)
		{
			renderPhysObject(*physObjects[i]);
		}

		dirLightShadowMaps[i].unbind();
	}
}

void ShadowMapRenderer::renderPhysObject(PhysObject& physObject)
{
	shadowMapMaterial.setWorldTransform(physObject.getAccumulatedTransform());
	shadowMapMaterial.updateObjectUniforms();
	shadowMapMaterial.bindInstance();

	physObject.meshComponent.mesh.bind();
	physObject.meshComponent.mesh.render();
}





/*		DeferredMaterial		*/


template<typename UniformBufferType>
static unsigned int createUniformBuffer()
{
	GLuint uniformBufferId = 0;
	OPENGL_CALL(glCreateBuffers(1, &uniformBufferId));

	if (uniformBufferId == 0)
	{
		assert(false);
		return 0;
	}

	OPENGL_CALL(glNamedBufferData(uniformBufferId, sizeof(UniformBufferType), nullptr, GL_DYNAMIC_DRAW));

	return uniformBufferId;
}

template<typename UniformBufferType>
static void updateUniformBlock(UniformBufferType& uniformBuffer, unsigned int uniformBufferId)
{
	void* uniformBufferPtr = nullptr;
	OPENGL_CALL((uniformBufferPtr = glMapNamedBufferRange(uniformBufferId, 0, sizeof(UniformBufferType), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)));
	if (uniformBufferPtr == nullptr)
	{
		GLenum error = glGetError();
		OutputDebugString(reinterpret_cast<const char*>(glewGetErrorString(error)));
		assert(false);
	}
	std::memcpy(uniformBufferPtr, &uniformBuffer, sizeof(UniformBufferType));
	OPENGL_CALL(glUnmapNamedBuffer(uniformBufferId));
}

static void bindTextureIfValid(GpuTexture gpuTexture, unsigned int textureUnit)
{
	if (gpuTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID) //TODO: introduce a "isValid" method, please
	{
		OPENGL_CALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, gpuTexture.textureId));
	}
	else
	{
		assert(false);
	}
}

GpuProgram DeferredMaterial::gpuProgram{};
unsigned int DeferredMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
DeferredMaterial::SceneVertexShaderUniformBlock DeferredMaterial::sceneVertexShaderUniformBlock{};

DeferredMaterial::DeferredMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("GBufferBuildProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "gBufferBuildVertexShader.glsl", g_shadersPath + "gBufferBuildFragmentShader.glsl");
			program.addFragmentShaderInclude(g_shadersPath + "normalMapping.glsl");
			g_programLibrary.add("GBufferBuildProgram", program);
		}

		gpuProgram = g_programLibrary.get("GBufferBuildProgram");
	}

	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
	}

	objectVertexShaderUniformBufferId = createUniformBuffer<ObjectVertexShaderUniformBlock>();
	objectFragmentShaderUniformBufferId = createUniformBuffer<ObjectFragmentShaderUniformBlock>();

	//TODO: add defaults
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate = glm::vec4{ 1.0f, 1.0f, 0.0f, 0.0f };
}

void DeferredMaterial::bind()
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));
}

void DeferredMaterial::bindInstance()const
{
	gpuProgram.bind();

	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, objectVertexShaderUniformBufferId, 0, sizeof(ObjectVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 2, objectFragmentShaderUniformBufferId, 0, sizeof(ObjectFragmentShaderUniformBlock)));

	bindTextureIfValid(diffuseMap, 0);
	bindTextureIfValid(normalMap, 1);
	bindTextureIfValid(specularMap, 2);
}

void DeferredMaterial::updateUniforms()
{
	//TODO: add dirty flags and checks	
	updateUniformBlock(objectVertexShaderUniformBlock, objectVertexShaderUniformBufferId);
	updateUniformBlock(objectFragmentShaderUniformBlock, objectFragmentShaderUniformBufferId);
}

void DeferredMaterial::setWorldTransform(const glm::mat4& world)
{
	objectVertexShaderUniformBlock.u_viewWorld = scene.camera.viewTransform * world;
}

void DeferredMaterial::setSpecularColor(const glm::vec3& specularColor)
{
	glm::vec4& currSpecular = objectFragmentShaderUniformBlock.u_matSpecularAndExponent;
	currSpecular = glm::vec4{ specularColor.x, specularColor.y, specularColor.z, currSpecular.w };
}

void DeferredMaterial::setSpecularExponent(float specularExponent)
{
	objectFragmentShaderUniformBlock.u_matSpecularAndExponent.w = specularExponent;
}

void DeferredMaterial::setTextCoordScale(const glm::vec2& textCoordScale)
{
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.x = textCoordScale.x;
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.y = textCoordScale.y;
}

void DeferredMaterial::setTextCoordTranslate(const glm::vec2& textCoordTranslate)
{
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.z = textCoordTranslate.x;
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.w = textCoordTranslate.y;
}

void DeferredMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_projection = scene.camera.projectionTransform;

	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
}




/*		DirLightDeferredShadingMaterial		*/

static void addLightingShaderSource(CpuProgram& program)
{
	ShaderSource defines;
	defines.shaderSource = "#define DIR_LIGHT_COUNT " + std::to_string(DIR_LIGHT_COUNT) +"\n";
	defines.shaderSource += "#define POINT_LIGHT_COUNT " + std::to_string(POINT_LIGHT_COUNT) + "\n";
#ifdef SHADOW_PCF
	defines.shaderSource += "#define SHADOW_PCF\n";
#elif defined(SHADOW_LERP)
	defines.shaderSource += "#define SHADOW_LERP\n";
#endif
	
	program.addVertexShaderInclude(defines);
	program.addFragmentShaderInclude(defines);
	program.addFragmentShaderInclude(g_shadersPath + "lighting.glsl");

}


GpuProgram DirLightDeferredShadingMaterial::gpuProgram{};

unsigned int DirLightDeferredShadingMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int DirLightDeferredShadingMaterial::sceneFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

DirLightDeferredShadingMaterial::SceneVertexShaderUniformBlock DirLightDeferredShadingMaterial::sceneVertexShaderUniformBlock{};
DirLightDeferredShadingMaterial::SceneFragmentShaderUniformBlock DirLightDeferredShadingMaterial::sceneFragmentShaderUniformBlock{};

GpuTexture DirLightDeferredShadingMaterial::dirShadowMaps[DIR_LIGHT_COUNT];

GpuTexture DirLightDeferredShadingMaterial::ssaoMap{};

GpuTexture DirLightDeferredShadingMaterial::depthBuffer{};
GpuTexture DirLightDeferredShadingMaterial::normalBuffer{};
GpuTexture DirLightDeferredShadingMaterial::diffuseBuffer{};
GpuTexture DirLightDeferredShadingMaterial::specularAndExponentBuffer{};

DirLightDeferredShadingMaterial::DirLightDeferredShadingMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("DirLightDeferredShadingProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "dirLightShadingVertexShader.glsl", g_shadersPath + "dirLightShadingFragmentShader.glsl");
			program.addFragmentShaderInclude(g_shadersPath + "positionReconstruction.glsl");
			//program.addFragmentShaderInclude(g_shadersPath + "lighting.glsl");
			addLightingShaderSource(program);
			g_programLibrary.add("DirLightDeferredShadingProgram", program);
		}

		gpuProgram = g_programLibrary.get("DirLightDeferredShadingProgram");
	}

	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
		sceneFragmentShaderUniformBufferId = createUniformBuffer<SceneFragmentShaderUniformBlock>();
	}
}

void DirLightDeferredShadingMaterial::bind()
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, sceneFragmentShaderUniformBufferId, 0, sizeof(SceneFragmentShaderUniformBlock)));

	bindTextureIfValid(depthBuffer, 0);
	bindTextureIfValid(normalBuffer, 1);
	bindTextureIfValid(diffuseBuffer, 2);
	bindTextureIfValid(specularAndExponentBuffer, 3);

	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		bindTextureIfValid(dirShadowMaps[i], 4 + i);
	}

	bindTextureIfValid(ssaoMap, 4 + DIR_LIGHT_COUNT);
}

void DirLightDeferredShadingMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_invProjection = scene.camera.invProjectionTransform;
	sceneFragmentShaderUniformBlock.u_ambientLight = scene.lighting->ambientLight;
	sceneFragmentShaderUniformBlock.u_frustumNear = scene.camera.nearPlane;
	sceneFragmentShaderUniformBlock.u_frustumFar = scene.camera.farPlane;

	glm::mat4 invViewTransform = scene.camera.invViewTransform;

	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		sceneFragmentShaderUniformBlock.u_directionalLights[i] = scene.lighting->directionalLights[i];

		glm::vec3& lightDir = sceneFragmentShaderUniformBlock.u_directionalLights[i].direction;
		glm::vec4 viewSpaceLightDir = scene.camera.viewTransform * glm::vec4(lightDir, 0.0f);
		lightDir = glm::normalize(glm::vec3{ viewSpaceLightDir.x, viewSpaceLightDir.y, viewSpaceLightDir.z });

		glm::vec4& shadowMapSizeAndBias = sceneFragmentShaderUniformBlock.u_dirShadowMapSizeAndBias[i];
		shadowMapSizeAndBias.x = static_cast<float>(scene.shadowMapRenderer->dirLightShadowMaps[i].width);
		shadowMapSizeAndBias.y = static_cast<float>(scene.shadowMapRenderer->dirLightShadowMaps[i].height);
		shadowMapSizeAndBias.z = scene.shadowMapRenderer->dirLightShadowMaps[i].bias;

		dirShadowMaps[i] = scene.shadowMapRenderer->dirLightShadowMaps[i].depthTexture;

		sceneFragmentShaderUniformBlock.u_dirShadowTransforms[i] = scene.shadowMapRenderer->dirLightProjectionViews[i] * invViewTransform;
	}

	depthBuffer = scene.deferredRenderer->gbuffer.depthTexture;
	normalBuffer = scene.deferredRenderer->gbuffer.normalTexture;
	diffuseBuffer = scene.deferredRenderer->gbuffer.diffuseTexture;
	specularAndExponentBuffer = scene.deferredRenderer->gbuffer.specularAndExponentTexture;

	ssaoMap = scene.deferredRenderer->ssaoRenderer->ssaoMap.ssaoTexture;

	updateUniformBlock(sceneFragmentShaderUniformBlock, sceneFragmentShaderUniformBufferId);
	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
}



/*		PointLightDeferredShadingMaterial		*/

GpuProgram PointLightDeferredShadingMaterial::gpuProgram{};

PointLightDeferredShadingMaterial::SceneVertexShaderUniformBlock PointLightDeferredShadingMaterial::sceneVertexShaderUniformBlock{};
PointLightDeferredShadingMaterial::SceneFragmentShaderUniformBlock PointLightDeferredShadingMaterial::sceneFragmentShaderUniformBlock{};

unsigned int PointLightDeferredShadingMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int PointLightDeferredShadingMaterial::sceneFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

GpuTexture PointLightDeferredShadingMaterial::depthBuffer{};
GpuTexture PointLightDeferredShadingMaterial::normalBuffer{};
GpuTexture PointLightDeferredShadingMaterial::diffuseBuffer{};
GpuTexture PointLightDeferredShadingMaterial::specularAndExponentBuffer{};

PointLightDeferredShadingMaterial::PointLightDeferredShadingMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("PointLightDeferredShadingProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "pointLightShadingVertexShader.glsl", g_shadersPath + "pointLightShadingFragmentShader.glsl");
			program.addFragmentShaderInclude(g_shadersPath + "positionReconstruction.glsl");
			//program.addFragmentShaderInclude(g_shadersPath + "lighting.glsl");
			addLightingShaderSource(program);
			g_programLibrary.add("PointLightDeferredShadingProgram", program);
		}

		gpuProgram = g_programLibrary.get("PointLightDeferredShadingProgram");
	}

	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
		sceneFragmentShaderUniformBufferId = createUniformBuffer<SceneFragmentShaderUniformBlock>();
	}
}

void PointLightDeferredShadingMaterial::bind()
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, sceneFragmentShaderUniformBufferId, 0, sizeof(SceneFragmentShaderUniformBlock)));

	bindTextureIfValid(depthBuffer, 0);
	bindTextureIfValid(normalBuffer, 1);
	bindTextureIfValid(diffuseBuffer, 2);
	bindTextureIfValid(specularAndExponentBuffer, 3);
}

void PointLightDeferredShadingMaterial::updateLightData(unsigned int pointLightIndex)
{
	PointLight& pointLight = scene.lighting->pointLights[pointLightIndex];

	glm::vec3 pointLightPos = glm::vec3{ pointLight.positionAndRadius.x, pointLight.positionAndRadius.y, pointLight.positionAndRadius.z };
	float pointLightRadius = pointLight.positionAndRadius.w;

	glm::mat4 lightWorldMatrix = glm::scale(glm::vec3{ pointLightRadius, pointLightRadius, pointLightRadius });
	lightWorldMatrix = glm::translate(pointLightPos) * lightWorldMatrix;

	sceneVertexShaderUniformBlock.u_viewWorld = scene.camera.viewTransform *lightWorldMatrix;

	sceneFragmentShaderUniformBlock.u_pointLight = pointLight;
	glm::vec4 viewSpaceLightPos = scene.camera.viewTransform * glm::vec4(pointLightPos, 1.0f);

	sceneFragmentShaderUniformBlock.u_pointLight.positionAndRadius = viewSpaceLightPos;
	sceneFragmentShaderUniformBlock.u_pointLight.positionAndRadius.w = pointLight.positionAndRadius.w;

	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
	updateUniformBlock(sceneFragmentShaderUniformBlock, sceneFragmentShaderUniformBufferId);
}

void PointLightDeferredShadingMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_projection = scene.camera.projectionTransform;
	sceneFragmentShaderUniformBlock.u_frustumNear = scene.camera.nearPlane;
	sceneFragmentShaderUniformBlock.u_frustumFar = scene.camera.farPlane;

	depthBuffer = scene.deferredRenderer->gbuffer.depthTexture;
	normalBuffer = scene.deferredRenderer->gbuffer.normalTexture;
	diffuseBuffer = scene.deferredRenderer->gbuffer.diffuseTexture;
	specularAndExponentBuffer = scene.deferredRenderer->gbuffer.specularAndExponentTexture;

	//here, we should update the uniform buffers but we don't since the scene and the per light data has been
	//kept together. this has been possible because the scene data is small.
}


/*		ForwardMaterial		*/

GpuProgram ForwardMaterial::gpuProgram{};

unsigned int ForwardMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int ForwardMaterial::sceneFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

ForwardMaterial::SceneVertexShaderUniformBlock ForwardMaterial::sceneVertexShaderUniformBlock{};
ForwardMaterial::SceneFragmentShaderUniformBlock ForwardMaterial::sceneFragmentShaderUniformBlock{};

GpuTexture ForwardMaterial::dirShadowMaps[DIR_LIGHT_COUNT];

ForwardMaterial::ForwardMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("ShipProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "forwardVertexShader.glsl", g_shadersPath + "forwardFragmentShader.glsl");
			//program.addFragmentShaderInclude(g_shadersPath + "lighting.glsl");
			addLightingShaderSource(program);
			program.addFragmentShaderInclude(g_shadersPath + "normalMapping.glsl");
			g_programLibrary.add("ShipProgram", program);
		}

		gpuProgram = g_programLibrary.get("ShipProgram");
	}

	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
		sceneFragmentShaderUniformBufferId = createUniformBuffer<SceneFragmentShaderUniformBlock>();
	}

	objectVertexShaderUniformBufferId = createUniformBuffer<ObjectVertexShaderUniformBlock>();
	objectFragmentShaderUniformBufferId = createUniformBuffer<ObjectFragmentShaderUniformBlock>();

	//TODO: add defaults
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate = glm::vec4{ 1.0f, 1.0f, 0.0f, 0.0f };
}

void ForwardMaterial::bind()
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 2, sceneFragmentShaderUniformBufferId, 0, sizeof(SceneFragmentShaderUniformBlock)));

	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		if (dirShadowMaps[i].textureId != INVALID_GRAPHICS_RESOURCE_ID)
		{
			OPENGL_CALL(glActiveTexture(GL_TEXTURE4 + i));
			OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, dirShadowMaps[i].textureId));
		}
	}
}

void ForwardMaterial::bindInstance()const
{
	gpuProgram.bind();

	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, objectVertexShaderUniformBufferId, 0, sizeof(ObjectVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 3, objectFragmentShaderUniformBufferId, 0, sizeof(ObjectFragmentShaderUniformBlock)));

	//here we assume that diffuseMap's binding index is 0
	bindTextureIfValid(diffuseMap, 0);

	//here we assume that normalMap's binding index is 1
	bindTextureIfValid(normalMap, 1);

	//same here with 2
	bindTextureIfValid(specularMap, 2);
}

void ForwardMaterial::updateUniforms()
{	
	//TODO: add dirty flags and checks	
	updateUniformBlock(objectVertexShaderUniformBlock, objectVertexShaderUniformBufferId);
	updateUniformBlock(objectFragmentShaderUniformBlock, objectFragmentShaderUniformBufferId);
}

void ForwardMaterial::setWorldTransform(const glm::mat4& world)
{
	objectVertexShaderUniformBlock.u_world = world;
}

void ForwardMaterial::setSpecularColor(const glm::vec3& specularColor)
{
	glm::vec4& currSpecular = objectFragmentShaderUniformBlock.u_matSpecularAndExponent;
	currSpecular = glm::vec4{ specularColor.x, specularColor.y, specularColor.z, currSpecular.w };
}

void ForwardMaterial::setSpecularExponent(float specularExponent)
{
	objectFragmentShaderUniformBlock.u_matSpecularAndExponent.w = specularExponent;
}

void ForwardMaterial::setTextCoordScale(const glm::vec2& textCoordScale)
{
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.x = textCoordScale.x;
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.y = textCoordScale.y;
}

void ForwardMaterial::setTextCoordTranslate(const glm::vec2& textCoordTranslate)
{
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.z = textCoordTranslate.x;
	objectFragmentShaderUniformBlock.u_textCoordScaleAndTranslate.w = textCoordTranslate.y;
}

void ForwardMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_projView = scene.camera.projectionViewTransform;

	sceneFragmentShaderUniformBlock.u_eyePosition = scene.camera.transform.pos;
	sceneFragmentShaderUniformBlock.u_ambientLight = scene.lighting->ambientLight;

	for (int i = 0; i < DIR_LIGHT_COUNT; ++i)
	{
		sceneVertexShaderUniformBlock.u_dirShadowTransforms[i] = scene.shadowMapRenderer->dirLightProjectionViews[i];

		sceneFragmentShaderUniformBlock.u_directionalLights[i] = scene.lighting->directionalLights[i];

		glm::vec4& shadowMapSizeAndBias = sceneFragmentShaderUniformBlock.u_dirShadowMapSizeAndBias[i];
		shadowMapSizeAndBias.x = static_cast<float>(scene.shadowMapRenderer->dirLightShadowMaps[i].width);
		shadowMapSizeAndBias.y = static_cast<float>(scene.shadowMapRenderer->dirLightShadowMaps[i].height);
		shadowMapSizeAndBias.z = scene.shadowMapRenderer->dirLightShadowMaps[i].bias;

		dirShadowMaps[i] = scene.shadowMapRenderer->dirLightShadowMaps[i].depthTexture;
	}

	for (int i = 0; i < POINT_LIGHT_COUNT; ++i)
	{
		sceneFragmentShaderUniformBlock.u_pointLights[i] = scene.lighting->pointLights[i];
	}

	updateUniformBlock(sceneFragmentShaderUniformBlock, sceneFragmentShaderUniformBufferId);
	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
}


/*		ShadowMap		*/

void ShadowMap::init(unsigned int width, unsigned int height)
{
	OPENGL_CALL(glGenFramebuffers(1, &frameBufferObjectId));
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));

	OPENGL_CALL(glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture.textureId));

	depthTexture.bind();

	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	resize(width, height);

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));//TODO: fetch current texture object id and rebind it here

	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture.textureId, 0));

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	OPENGL_CALL(glDrawBuffer(GL_NONE));

	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0)); //TODO: fetch current frame buffer object id and rebind it here
}

void ShadowMap::resize(unsigned int _width, unsigned int _height)
{
	if (_width == width && _height == height)
	{
		return;
	}

	assert(depthTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);
	depthTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0)); //TODO: fetch current texture id and rebind it here

	width = _width;
	height = _height;
}

void ShadowMap::bindAsDepthBuffer()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));

	OPENGL_CALL(glClear(GL_DEPTH_BUFFER_BIT));

	OPENGL_CALL(glViewport(0, 0, width, height));
}

void ShadowMap::bindAsSampler()const
{
	depthTexture.bind();
}

void ShadowMap::unbind()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void ShadowMap::release()
{
	if (frameBufferObjectId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteFramebuffers(1, &frameBufferObjectId));
		frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;
	}

	depthTexture.release();
}

/*		ShadowMapMaterial		*/

GpuProgram ShadowMapMaterial::gpuProgram{};

ShadowMapMaterial::ShadowMapMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("ShadowMapProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "ShadowMapVertexShader.glsl", g_shadersPath + "ShadowMapFragmentShader.glsl");
			g_programLibrary.add("ShadowMapProgram", program);
		}

		gpuProgram = g_programLibrary.get("ShadowMapProgram");
	}

	lightVertexShaderUniformBufferId = createUniformBuffer<LightVertexShaderUniformBlock>();
	objectVertexShaderUniformBufferId = createUniformBuffer<ObjectVertexShaderUniformBlock>();
}

void ShadowMapMaterial::setWorldTransform(const glm::mat4& world)
{
	objectVertexShaderUniformBlock.u_world = world;
}

void ShadowMapMaterial::bindInstance()const
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, lightVertexShaderUniformBufferId, 0, sizeof(LightVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, objectVertexShaderUniformBufferId, 0, sizeof(ObjectVertexShaderUniformBlock)));
}

void ShadowMapMaterial::updateObjectUniforms()
{
	//TODO: add dirty flags and checks	
	updateUniformBlock(objectVertexShaderUniformBlock, objectVertexShaderUniformBufferId);
}

void ShadowMapMaterial::updateLightUniforms()
{	
	updateUniformBlock(lightVertexShaderUniformBlock, lightVertexShaderUniformBufferId);
}

void ShadowMapMaterial::setLightProjectionView(const glm::mat4& lightProjectionView)
{
	lightVertexShaderUniformBlock.u_projView = lightProjectionView;
}

const glm::mat4& ShadowMapMaterial::getLightProjectionView()const
{
	return lightVertexShaderUniformBlock.u_projView;
}

/*		SSAOMaterial		*/

GpuProgram SSAOMaterial::gpuProgram{};
SSAOMaterial::SceneVertexShaderUniformBlock SSAOMaterial::sceneVertexShaderUniformBlock{};
SSAOMaterial::SceneFragmentShaderUniformBlock SSAOMaterial::sceneFragmentShaderUniformBlock{};
SSAOMaterial::ConstantFragmentShaderUniformBlock SSAOMaterial::constantFragmentShaderUniformBlock{};

unsigned int SSAOMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int SSAOMaterial::sceneFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int SSAOMaterial::constantFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

GpuTexture SSAOMaterial::depthBuffer{};
GpuTexture SSAOMaterial::normalBuffer{};
GpuTexture SSAOMaterial::randomDirectionsTexture{};

SSAOMaterial::SSAOMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("SSAOProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "ssaoVertexShader.glsl", g_shadersPath + "ssaoFragmentShader.glsl");
			program.addFragmentShaderInclude(g_shadersPath + "positionReconstruction.glsl");

			ShaderSource defines;
			defines.shaderSource = "#define SAMPLES_COUNT " + std::to_string(SSAO_SAMPLES_COUNT) + "\n";
			defines.shaderSource += "#define OCCLUSION_RADIUS " + std::to_string(SSAO_OCCLUSION_RADIUS) + "\n";
			defines.shaderSource += "#define EPSILON " + std::to_string(SSAO_EPSILON) + "\n";
			defines.shaderSource += "#define MIN_DISTANCE " + std::to_string(SSAO_MIN_DISTANCE) + "\n";
			defines.shaderSource += "#define MAX_DISTANCE " + std::to_string(SSAO_MAX_DISTANCE) + "\n";

			program.addFragmentShaderInclude(defines);

			g_programLibrary.add("SSAOProgram", program);
		}

		gpuProgram = g_programLibrary.get("SSAOProgram");
	}
		
	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
		sceneFragmentShaderUniformBufferId = createUniformBuffer<SceneFragmentShaderUniformBlock>();
		constantFragmentShaderUniformBufferId = createUniformBuffer<ConstantFragmentShaderUniformBlock>();
		
		sceneFragmentShaderUniformBlock.u_randomDirectionTiling = glm::vec2{ 2.0f, 2.0f };

		//uniformly distributed vectors: corners and face centers of the [-1, 1]^3 cube
		glm::vec4* sampleOffsets = constantFragmentShaderUniformBlock.u_sampleOffsets;

		//corners
		sampleOffsets[0] = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		sampleOffsets[1] = glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f);

		sampleOffsets[2] = glm::vec4(-1.0f, 1.0f, 1.0f, 0.0f);
		sampleOffsets[3] = glm::vec4(1.0f, -1.0f, -1.0f, 0.0f);

		sampleOffsets[4] = glm::vec4(1.0f, 1.0f, -1.0f, 0.0f);
		sampleOffsets[5] = glm::vec4(-1.0f, -1.0f, 1.0f, 0.0f);

		sampleOffsets[6] = glm::vec4(-1.0f, 1.0f, -1.0f, 0.0f);
		sampleOffsets[7] = glm::vec4(1.0f, -1.0f, 1.0f, 0.0f);

		//face centers
		sampleOffsets[8] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);
		sampleOffsets[9] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

		sampleOffsets[10] = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
		sampleOffsets[11] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

		sampleOffsets[12] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		sampleOffsets[13] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

		for (int i = 0; i < SSAO_SAMPLES_COUNT; ++i)
		{	
			sampleOffsets[i] = glm::linearRand<float>(0.25, 1.0f) * glm::normalize(sampleOffsets[i]);
		}

		updateUniformBlock(constantFragmentShaderUniformBlock, constantFragmentShaderUniformBufferId);		
	}

	if (randomDirectionsTexture.textureId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_textureLibrary.exists("RandomDirectionsTexture"))
		{
			CpuTexture randomDirTexture;
			randomDirTexture.createRandom(kRandomDirectionTextureSize);
			randomDirTexture.isLinear = true;
			g_textureLibrary.add("RandomDirectionsTexture", randomDirTexture);
		}

		randomDirectionsTexture = g_textureLibrary.get("RandomDirectionsTexture");
	}
}

void SSAOMaterial::bind()
{
	gpuProgram.bind();

	//here we assume the binding index for the uniform blocks
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, sceneFragmentShaderUniformBufferId, 0, sizeof(SceneFragmentShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 2, constantFragmentShaderUniformBufferId, 0, sizeof(ConstantFragmentShaderUniformBlock)));

	bindTextureIfValid(depthBuffer, 0);
	bindTextureIfValid(normalBuffer, 1);
	bindTextureIfValid(randomDirectionsTexture, 2);	
}

void SSAOMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_invProjection = scene.camera.invProjectionTransform;
	sceneFragmentShaderUniformBlock.u_projection = scene.camera.projectionTransform;
	sceneFragmentShaderUniformBlock.u_frustumNear = scene.camera.nearPlane;
	sceneFragmentShaderUniformBlock.u_frustumFar = scene.camera.farPlane;
	
	depthBuffer = scene.deferredRenderer->gbuffer.depthTexture;

	//we should not use the normal-mapped normal buffer, but for this particular use case
	//it's not worth using another buffer and therefore more bandwidth
	normalBuffer = scene.deferredRenderer->gbuffer.normalTexture;

	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
	updateUniformBlock(sceneFragmentShaderUniformBlock, sceneFragmentShaderUniformBufferId);		
}

/*		SSAOMap		*/

void SSAOMap::init(unsigned int width, unsigned int height)
{
	OPENGL_CALL(glGenFramebuffers(1, &frameBufferObjectId));
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));
	
	OPENGL_CALL(glCreateTextures(GL_TEXTURE_2D, 1, &ssaoTexture.textureId));

	ssaoTexture.bind();

	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	OPENGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		
	resize(width, height);

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));//TODO: fetch current texture object id and rebind it here

	OPENGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ssaoTexture.textureId, 0));

	OPENGL_CALL(glCreateRenderbuffers(1, &renderBufferObjectId));
	OPENGL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, renderBufferObjectId));
	OPENGL_CALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
	
	OPENGL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBufferObjectId));

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	GLenum colorBuffers[] = { GL_COLOR_ATTACHMENT0 };
	OPENGL_CALL(glDrawBuffers(1, colorBuffers));

	OPENGL_CALL(glClearColor(1.0f, 0.0f, 0.0f, 0.0f));
	OPENGL_CALL(glClear(GL_COLOR_BUFFER_BIT));
		
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0)); //TODO: fetch current frame buffer object id and rebind it here
}

void SSAOMap::resize(unsigned int _width, unsigned int _height)
{
	if (_width == width && _height == height)
	{
		return;
	}

	assert(ssaoTexture.textureId != INVALID_GRAPHICS_RESOURCE_ID);

	ssaoTexture.bind();
	OPENGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, _width, _height, 0, GL_RGB, GL_FLOAT, nullptr));

	OPENGL_CALL(glBindTexture(GL_TEXTURE_2D, 0)); //TODO: fetch current texture id and rebind it here

	width = _width;
	height = _height;
}

void SSAOMap::bind()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObjectId));

	OPENGL_CALL(glClearColor(1.0f, 0.0f, 0.0f, 0.0f));
	OPENGL_CALL(glClear(GL_COLOR_BUFFER_BIT));

	OPENGL_CALL(glViewport(0, 0, width, height));
}

void SSAOMap::unbind()const
{
	OPENGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void SSAOMap::release()
{	
	if (frameBufferObjectId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteFramebuffers(1, &frameBufferObjectId));
		frameBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;		
	}

	if (renderBufferObjectId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glDeleteRenderbuffers(1, &renderBufferObjectId));
		renderBufferObjectId = INVALID_GRAPHICS_RESOURCE_ID;
	}

	ssaoTexture.release();
}


/*		SSAORenderer		*/

SSAORenderer::SSAORenderer()
{
	ssaoMap.init(windowWidth, windowHeight);
	blurIntermediateBuffer.init(windowWidth, windowHeight);

	fullScreenQuad = getFullScreenQuad();
}

void SSAORenderer::render()const
{
	OPENGL_CALL(glDisable(GL_DEPTH_TEST));

	SSAOMaterial::updateSceneData();
	SSAOMaterial::bind();

	ssaoMap.bind();

	fullScreenQuad.bind();
	fullScreenQuad.render();
	
	ssaoMap.unbind();
		
	EdgePreservingBlurMaterial::setTexelSize(glm::vec2(1.0f/ windowWidth, 1.0f/ windowHeight));
	EdgePreservingBlurMaterial::updateSceneData();
	
	EdgePreservingBlurMaterial::colorMap = ssaoMap.ssaoTexture;

	EdgePreservingBlurMaterial::bindHorizontal();

	blurIntermediateBuffer.bind();

	fullScreenQuad.bind();
	fullScreenQuad.render();

	blurIntermediateBuffer.unbind();

	EdgePreservingBlurMaterial::colorMap = blurIntermediateBuffer.ssaoTexture;

	EdgePreservingBlurMaterial::bindVertical();

	ssaoMap.bind();

	fullScreenQuad.bind();
	fullScreenQuad.render();

	ssaoMap.unbind();
	
	OPENGL_CALL(glEnable(GL_DEPTH_TEST));
}


/*		EdgePreservingBlurMaterial		*/

GpuProgram EdgePreservingBlurMaterial::horizontalGpuProgram{};
GpuProgram EdgePreservingBlurMaterial::verticalGpuProgram{};
EdgePreservingBlurMaterial::SceneFragmentShaderUniformBlock EdgePreservingBlurMaterial::sceneFragmentShaderUniformBlock{};
EdgePreservingBlurMaterial::ConstantFragmentShaderUniformBlock EdgePreservingBlurMaterial::constantFragmentShaderUniformBlock{};

unsigned int EdgePreservingBlurMaterial::sceneFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
unsigned int EdgePreservingBlurMaterial::constantFragmentShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;

GpuTexture EdgePreservingBlurMaterial::depthBuffer{};
GpuTexture EdgePreservingBlurMaterial::normalBuffer{};
GpuTexture EdgePreservingBlurMaterial::colorMap{};

static std::vector<float> computeGaussianWeights(float sigma, int kernelRadius)
{
	std::vector<float> weights(kernelRadius * 2 + 1);
	const float twoSquaredSigma = sigma*sigma * 2.0f;

	float weightSum = 0.0f;

	for (int x = -kernelRadius; x <= kernelRadius; ++x)
	{
		float weight = expf(-x*x / twoSquaredSigma);
		weights[x + kernelRadius] = weight;
		weightSum += weight;
	}

	for (unsigned int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

EdgePreservingBlurMaterial::EdgePreservingBlurMaterial()
{
	ShaderSource defines;
	defines.shaderSource = "#define KERNEL_RADIUS " + std::to_string(BLUR_KERNEL_RADIUS) + "\n";
		
	//weights are packed in vec4 objects
	const unsigned int packedWeightsCount = static_cast<unsigned int >(std::ceilf(static_cast<float>(BLUR_KERNEL_RADIUS * 2 + 1) / 4.0f));

	ShaderSource constantFragmentShaderUniformBlockDecl;
	constantFragmentShaderUniformBlockDecl.shaderSource = "layout(binding = 1, std140) uniform ConstantFragmentShaderUniformBlock\n";
	constantFragmentShaderUniformBlockDecl.shaderSource += "{\n";
	constantFragmentShaderUniformBlockDecl.shaderSource += "vec4 u_packedWeights["+ std::to_string(packedWeightsCount) +"];\n";
	constantFragmentShaderUniformBlockDecl.shaderSource += "};\n";

	ShaderSource unpackedWeightsDecl;
	unpackedWeightsDecl.shaderSource = "float weights[KERNEL_RADIUS*2+1]=\n";
	unpackedWeightsDecl.shaderSource += "{\n";
		unpackedWeightsDecl.shaderSource += "u_packedWeights[0].x, u_packedWeights[0].y, u_packedWeights[0].z, u_packedWeights[0].w, u_packedWeights[1].x,\n";
		unpackedWeightsDecl.shaderSource += "u_packedWeights[1].y,\n";
		unpackedWeightsDecl.shaderSource += "u_packedWeights[1].z, u_packedWeights[1].w, u_packedWeights[2].x, u_packedWeights[2].y, u_packedWeights[2].z\n";
	unpackedWeightsDecl.shaderSource += "};\n";
	

	if (horizontalGpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("EdgePreservingHorizontalBlurProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "blurVertexShader.glsl", g_shadersPath + "edgePreservingBlurFragmentShader.glsl");
			
			program.addFragmentShaderInclude(g_shadersPath + "positionReconstruction.glsl");
			program.addFragmentShaderInclude(defines);
			program.addFragmentShaderInclude(constantFragmentShaderUniformBlockDecl);
			program.addFragmentShaderInclude(unpackedWeightsDecl);

			ShaderSource define;
			define.shaderSource = "#define HORIZONTAL\n";
			program.addFragmentShaderInclude(define);

			g_programLibrary.add("EdgePreservingHorizontalBlurProgram", program);
		}

		horizontalGpuProgram = g_programLibrary.get("EdgePreservingHorizontalBlurProgram");
	}

	if (verticalGpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("EdgePreservingVerticalBlurProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "blurVertexShader.glsl", g_shadersPath + "edgePreservingBlurFragmentShader.glsl");

			program.addFragmentShaderInclude(g_shadersPath + "positionReconstruction.glsl");
			program.addFragmentShaderInclude(defines);
			program.addFragmentShaderInclude(constantFragmentShaderUniformBlockDecl);
			program.addFragmentShaderInclude(unpackedWeightsDecl);

			g_programLibrary.add("EdgePreservingVerticalBlurProgram", program);
		}

		verticalGpuProgram = g_programLibrary.get("EdgePreservingVerticalBlurProgram");
	}

	if (sceneFragmentShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneFragmentShaderUniformBufferId = createUniformBuffer<SceneFragmentShaderUniformBlock>();
		constantFragmentShaderUniformBufferId = createUniformBuffer<ConstantFragmentShaderUniformBlock>();

		std::vector<float> blurWeights = computeGaussianWeights(1.0f, BLUR_KERNEL_RADIUS);
		std::memcpy(constantFragmentShaderUniformBlock.u_weights, blurWeights.data(), blurWeights.size() * sizeof(float));
		
		updateUniformBlock(constantFragmentShaderUniformBlock, constantFragmentShaderUniformBufferId);
	}
}

void EdgePreservingBlurMaterial::bindHorizontal()
{
	horizontalGpuProgram.bind();
	bind();
}

void EdgePreservingBlurMaterial::bindVertical()
{
	verticalGpuProgram.bind();
	bind();
}

void EdgePreservingBlurMaterial::bind()
{
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneFragmentShaderUniformBufferId, 0, sizeof(SceneFragmentShaderUniformBlock)));
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 1, constantFragmentShaderUniformBufferId, 0, sizeof(ConstantFragmentShaderUniformBlock)));

	bindTextureIfValid(depthBuffer, 0);
	bindTextureIfValid(normalBuffer, 1);
	bindTextureIfValid(colorMap, 2);
}

void EdgePreservingBlurMaterial::setTexelSize(glm::vec2 texelSize)
{
	sceneFragmentShaderUniformBlock.u_texelSize = texelSize;
}

void EdgePreservingBlurMaterial::updateSceneData()
{	
	sceneFragmentShaderUniformBlock.u_frustumNear = scene.camera.nearPlane;
	sceneFragmentShaderUniformBlock.u_frustumFar = scene.camera.farPlane;

	depthBuffer = scene.deferredRenderer->gbuffer.depthTexture;
	normalBuffer = scene.deferredRenderer->gbuffer.normalTexture;

	updateUniformBlock(sceneFragmentShaderUniformBlock, sceneFragmentShaderUniformBufferId);
}


/*		SkyBoxMaterial		*/

GpuProgram SkyBoxMaterial::gpuProgram{};
SkyBoxMaterial::SceneVertexShaderUniformBlock SkyBoxMaterial::sceneVertexShaderUniformBlock{};
unsigned int SkyBoxMaterial::sceneVertexShaderUniformBufferId = INVALID_GRAPHICS_RESOURCE_ID;
#ifndef FORWARD_RENDER
GpuTexture SkyBoxMaterial::depthBuffer{};
#endif

SkyBoxMaterial::SkyBoxMaterial()
{
	if (gpuProgram.programId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		if (!g_programLibrary.exists("SkyBoxProgram"))
		{
			CpuProgram program;
			program.import(g_shadersPath + "skyBoxVertexShader.glsl", g_shadersPath + "skyBoxFragmentShader.glsl");

#ifndef FORWARD_RENDER
			ShaderSource shaderSource;
			shaderSource.shaderSource = "#define DEFERRED_RENDER\n";
			program.addVertexShaderInclude(shaderSource);
			program.addFragmentShaderInclude(shaderSource);
#endif

#ifdef SKYBOX_SWAP_SKYBOX_YZ
			ShaderSource define;
			define.shaderSource = "#define SWAP_SKYBOX_YZ\n";
			program.addFragmentShaderInclude(define);
#endif

			g_programLibrary.add("SkyBoxProgram", program);
		}

		gpuProgram = g_programLibrary.get("SkyBoxProgram");
	}

	if (sceneVertexShaderUniformBufferId == INVALID_GRAPHICS_RESOURCE_ID)
	{
		sceneVertexShaderUniformBufferId = createUniformBuffer<SceneVertexShaderUniformBlock>();
	}
}

void SkyBoxMaterial::bind()
{
	gpuProgram.bind();
	OPENGL_CALL(glBindBufferRange(GL_UNIFORM_BUFFER, 0, sceneVertexShaderUniformBufferId, 0, sizeof(SceneVertexShaderUniformBlock)));

#ifndef FORWARD_RENDER
	bindTextureIfValid(depthBuffer, 1);
#endif
}

void SkyBoxMaterial::bindInstance()const
{
	gpuProgram.bind();

	if (skyBox.textureId != INVALID_GRAPHICS_RESOURCE_ID)
	{
		OPENGL_CALL(glActiveTexture(GL_TEXTURE0));
		OPENGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, skyBox.textureId));
	}
}

void SkyBoxMaterial::updateSceneData()
{
	sceneVertexShaderUniformBlock.u_invProjection = scene.camera.invProjectionTransform;
	sceneVertexShaderUniformBlock.u_invView = scene.camera.invViewTransform;

#ifndef FORWARD_RENDER
	depthBuffer = scene.deferredRenderer->gbuffer.depthTexture;
#endif

	updateUniformBlock(sceneVertexShaderUniformBlock, sceneVertexShaderUniformBufferId);
}

/*		SkyBoxRenderer		*/

SkyBoxRenderer::SkyBoxRenderer()
{
	fullScreenQuad = getFullScreenQuad();
	glEnable(GL_TEXTURE_CUBE_MAP);
}

void SkyBoxRenderer::render()const
{
	SkyBoxMaterial::updateSceneData();
	SkyBoxMaterial::bind();


	skyBoxMaterial.bindInstance();

#ifndef FORWARD_RENDER
	glDisable(GL_DEPTH_TEST);
#endif

	fullScreenQuad.bind();
	fullScreenQuad.render();

#ifndef FORWARD_RENDER
	glEnable(GL_DEPTH_TEST);
#endif
}