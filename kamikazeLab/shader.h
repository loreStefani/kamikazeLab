#ifndef _SHADER_H_
#define _SHADER_H_

#include <string>
#include <vector>
#include "asset_library.h"
#include "graphics_resource.h"

struct GpuProgram
{	
	unsigned int programId = INVALID_GRAPHICS_RESOURCE_ID;
	void bind()const;	

	void release();
};

struct ShaderSource
{
	bool import(const std::string& shaderSourcePath);
	std::string shaderSource;
};

struct CpuProgram
{
	bool import(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath);

	bool addVertexShaderInclude(const std::string& shaderSourceFilePath, bool append = true);
	bool addFragmentShaderInclude(const std::string& shaderSourceFilePath, bool append = true);

	void addVertexShaderInclude(const ShaderSource& shaderSource, bool append = true);
	void addFragmentShaderInclude(const ShaderSource& shaderSource, bool append = true);

	GpuProgram uploadToGPU() const;

	std::vector<std::string> vertexShaderSources;	
	std::vector<std::string> fragmentShaderSources;	
};

using ProgramLibrary = AssetLibrary<CpuProgram, GpuProgram>;

extern ProgramLibrary g_programLibrary;

#endif