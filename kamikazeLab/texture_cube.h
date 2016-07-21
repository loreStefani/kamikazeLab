#ifndef _TEXTURE_CUBE_H_
#define _TEXTURE_CUBE_H_

#include "texture.h"

using GpuTextureCube = GpuTexture;

struct CpuTextureCube
{
	//in order:
	//positive x face, negative x face,
	//positive y face, negative y face,
	//positive z face, negative z face,
	bool import(const std::vector<std::string>& cubeFacesFileNames);

	int sizeX, sizeY;

	using Texels = std::vector<Texel>;

	std::vector<Texels> facesData;

	bool isLinear = false;
	bool generateMipMaps = false;

	GpuTextureCube uploadToGPU() const;
};

using TextureCubeLibrary = AssetLibrary<CpuTextureCube, GpuTextureCube>;

extern TextureCubeLibrary g_textureCubeLibrary;

#endif
