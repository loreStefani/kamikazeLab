#ifndef MESH_H
#define MESH_H

/* Mesh asset: a indexed, triangular Mesh
 *
 * Used for rendering only (graphic coating).
 *
 * Like many assets, two classes:
 * - CpuMesh ("stored in system ram")
 * - GpuMesh ("stored in graphic card RAM")
 *
 * MeshComponent:
 * - a mesh + a texture (will be: a material) + a transform
 *
 */

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
using namespace glm;

#include "transform.h"
#include "texture.h"

#include "asset_library.h"

#include "render_path.h"
#ifdef FORWARD_RENDER
#include "forward_material.h"
#else
#include "deferred_materials.h"
#endif

#include "graphics_resource.h"


struct Vertex{	
	vec3 pos;
	vec3 norm;
	vec2 uv;
	vec4 tang; //w is the tangent frame handedness
};

struct Tri{
	int i,j,k;
	Tri(){};
	Tri(int _i, int _j, int _k):i(_i),j(_j),k(_k){}
};

/* (ids of) a mesh structure in GPU ram */
struct GpuMesh{
	void bind()const;
	void render() const;
	uint geomBufferId = INVALID_GRAPHICS_RESOURCE_ID; // "name" of GPU buffer for vertices
	uint connBufferId = INVALID_GRAPHICS_RESOURCE_ID; // "name" of GPU buffer for triangles
	uint vertexArrayId = INVALID_GRAPHICS_RESOURCE_ID;
	int nElements = 0;

	void release();
};

/* a mesh structure in CPU ram */
struct CpuMesh{
	std::vector< Vertex > verts;
	std::vector< Tri > tris;

	bool import(const std::string& filename);
	void renderDeprecated(); // uses immediate mode

	// todo: a bit of geometry processing...
	void updateNormals();
	void resize( float scale );
	void flipYZ();
	void apply(Transform t);

	// procedural constructions..
	void buildTorus(int ni, int nj, float innerRadius, float outerRadius);
	void buildGrid(float width, float depth, int m, int n);
	void buildSphere(float radius, int sliceCount, int stackCount);
	void buildFullScreenQuad(); //built in ndc space

	GpuMesh uploadToGPU()const;

private:
	void addQuad(int i, int j, int k, int h);
	void computeTangents();
};

struct MeshComponent{
	Transform t; /* local trasform, for the looks only
				  * useful for: convert unity of measures between assets and game,
				  * add small "graphic only" animations,
				  * fix axis orientations between assets and game
				  */

	GpuMesh mesh;
	// textures, materials...
#ifdef FORWARD_RENDER
	ForwardMaterial material;
#else
	DeferredMaterial material;
#endif
};

using MeshLibrary = AssetLibrary<CpuMesh, GpuMesh>;

extern MeshLibrary g_meshLibrary;


#endif // MESH_H
