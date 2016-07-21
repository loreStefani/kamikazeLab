#ifndef TEXTURE_H
#define TEXTURE_H

/* class Texture:
 *
 * an asset for a texure map.
 *
 */
#include <string>
#include <vector>
#include "asset_library.h"
#include "graphics_resource.h"

struct GpuTexture{
	unsigned int textureId = INVALID_GRAPHICS_RESOURCE_ID;
	void bind() const;
	void release();
};

typedef unsigned char byte;

struct Texel{
	byte r,g,b,a;
};

struct CpuTexture{
	bool import(std::string filename);

	int sizeX, sizeY;
	std::vector<Texel> data;
	bool isLinear = false;
	bool generateMipMaps = true;

	GpuTexture uploadToGPU() const;

	// procedura creation of textures!
	void createRandom(int size);

	// TODO: un po' di image processing, maybe?
	void sharpen();
	void blur();
	void tuneHueSaturationBrightness();
	void turnIntoANormalMap();  // from a displacement map
};

using TextureLibrary = AssetLibrary<CpuTexture, GpuTexture>;

extern TextureLibrary g_textureLibrary;

#endif // TEXTURE_H
