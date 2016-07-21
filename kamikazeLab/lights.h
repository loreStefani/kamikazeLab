#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include <glm\glm.hpp>

#define DIR_LIGHT_COUNT 1
#define POINT_LIGHT_COUNT 10

//shadow filtering techniques, undefine both to use the trivial way
#define SHADOW_LERP
//#define SHADOW_PCF

#ifndef DIR_LIGHT_COUNT 
#define DIR_LIGHT_COUNT 0
#endif

#ifndef POINT_LIGHT_COUNT 
#define POINT_LIGHT_COUNT 0
#endif

static_assert(DIR_LIGHT_COUNT > 0, "Invalid directional lights count!");
static_assert(POINT_LIGHT_COUNT > 0, "Invalid point lights count!");

struct DirectionalLight
{
	glm::vec3 color;
	float pad;
	glm::vec3 direction;
	float pad1;
};

struct PointLight
{
	glm::vec4 positionAndRadius;
	glm::vec3 color;
	float pad;
	glm::vec3 attenuation;
	float pad1;
};


struct SceneLighting
{
	DirectionalLight directionalLights[DIR_LIGHT_COUNT];
	PointLight pointLights[POINT_LIGHT_COUNT];	
	glm::vec3 ambientLight;	
};

#endif