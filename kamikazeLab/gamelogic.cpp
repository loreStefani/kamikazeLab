/* game_logic.cpp
 * all methods with are specific of this game.
 * (in Unity, they would probaby be custom scripts)
 */

#include "custom_classes.h"
#include <glm/gtx/transform.hpp>
#include "window.h"
#include "deferred_renderer.h"
#include "forward_renderer.h"
#include "shadow_map_renderer.h"
#include "skybox_renderer.h"
#include "render_path.h"

Scene scene;

void Ship::setMaxVelAndAcc(float maxSpeed, float acc){
	stats.accRate = acc;

	// compute drag so that limit speed is maxSpeed
	drag = acc / maxSpeed;
}

float randInZeroToOne(){
	return ((rand()%1001))/1000.0f;
}

float randInMinusOneToOne(){
	return ((rand()%2001)-1000)/1000.0f;
}


vec3 randomFlatUnitVec(){
	return vec3(
				randInMinusOneToOne(),
				randInMinusOneToOne(),
				0
				);
}

vec3 randomUnitVec(){
	vec3 res(
				randInMinusOneToOne(),
				randInMinusOneToOne(),
				randInMinusOneToOne()
				);
	glm::normalize(res); return res;
}

void Ship::reset(){
	timeBeforeFiringAgain = 0.0; // ready!
	PhysObject::reset();
	for (Bullet &b : bullets) b.reset();

	t.pos = scene.randomPosInArena();
	angDrag =0.2f/(1.0f/30);
	alive = true;
}

void Ship::respawn(){
	reset();
}

void Ship::die(){
	if (!alive) return;
	alive = false;
	timeDead = 0;
	angVel = glm::angleAxis( (rand()%200+100)/400.0f, randomUnitVec());
	glm::normalize(angVel);
	angDrag = 0.0;
}

vec3 Scene::randomPosInArena() const{
	return randomFlatUnitVec() *arenaRadius;
}


Bullet& Ship::findUnusedBullet(){
	for (Bullet &b : bullets) {
		if (!b.alive) return b;
	}
	return bullets[0]; // this should never happen
}

void Ship::spawnNewBullet(){
	Bullet &b = findUnusedBullet();
	fillBullet( b );
}

void Ship::fillBullet(Bullet &b) const {
	b.alive = true;
	b.t.pos = t.pos; // TODO: put where the gun hole is (in Space shape)
	b.t.ori = t.ori;

	b.timeToLive = stats.fireRange / stats.fireSpeed;
	b.vel = t.forward() * stats.fireSpeed + 0.3f*vel;
	b.angVel = quat(1,0,0,0);

	b.mass = 0.1f;
	b.coll.radius = 0.03f;
	// TODO: maybe randomize a bit pos and vel	

}

bool Scene::isInside( vec3 p ) const{
	return ( p.x>=-arenaRadius && p.x<=arenaRadius &&
			 p.y>=-arenaRadius && p.y<=arenaRadius );
}

vec3 Scene::pacmanWarp( vec3 p) const{
	vec3 res = p;
	if (res.x>+arenaRadius) res.x -= arenaRadius*2;
	if (res.y>+arenaRadius) res.y -= arenaRadius*2;
	if (res.x<-arenaRadius) res.x += arenaRadius*2;
	if (res.y<-arenaRadius) res.y += arenaRadius*2;
	return res;
}

void Scene::initAsNewGame(){

	arenaRadius = 60;

	physObjects.clear();
	physObjects.reserve(2 + 2 * 100 + 1);

	ships.resize(2);
		
	for (Ship &s: ships) {
		s.bullets.resize( 100 ); // move in init

		for (Bullet& b : s.bullets)
		{
			// let set a tranform manually to adapt the bullet asset to our needs
			b.meshComponent.t.scale = 0.5f;
		}

		s.reset();

		s.coll.radius = 0.8f;
		s.mass = 10.0; // KG!
						
		// let set a tranform manually to adapt the ship asset to our needs
		s.meshComponent.t.setIde();
		s.meshComponent.t.scale = 0.05f;
		s.meshComponent.t.ori = quat(-sqrt(2.0f) / 2.0f, 0, 0, sqrt(2.0f) / 2.0f);
	}
	ships[0].setStatsAsFighter();
	ships[1].setStatsAsTank();
	
	floor.reset(new Floor());	
	floor->meshComponent.t.ori = glm::angleAxis(glm::pi<float>()*0.5f, glm::vec3{ 1.0f, 0.0f, 0.0f });
	floor->meshComponent.t.scale = 2.0f*arenaRadius;

	floor->t.pos = glm::vec3{ 0.0f, 0.0f, -1.2f };

	camera.setProjectionParams(glm::pi<float>() * 0.45f, static_cast<float>(windowWidth) / windowHeight, 1.0f, 100.0f);
	camera.computeInvProj();
	camera.viewTransform = glm::mat4();

	lighting.reset(new SceneLighting{});
	lighting->ambientLight = glm::vec3(0.1f, 0.1f, 0.1f);
		
	lighting->directionalLights[0].color = glm::vec3{ 0.5f, 0.5f, 0.5f };
	lighting->directionalLights[0].direction = glm::normalize(glm::vec3{ -0.5f, 0.0f, -1.0f });
	
	constexpr float arenaLightsRadius = 4.0f;
	lighting->pointLights[2].positionAndRadius = glm::vec4{ -arenaRadius, arenaRadius, 0.0f, arenaLightsRadius};
	lighting->pointLights[3].positionAndRadius = glm::vec4{ 0.0f, arenaRadius, 0.0f, arenaLightsRadius };
	lighting->pointLights[4].positionAndRadius = glm::vec4{ arenaRadius, arenaRadius, 0.0f, arenaLightsRadius };
	lighting->pointLights[5].positionAndRadius = glm::vec4{ -arenaRadius, 0.0f, 0.0f, arenaLightsRadius };
	lighting->pointLights[6].positionAndRadius = glm::vec4{ arenaRadius, 0.0f, 0.0f, arenaLightsRadius };
	lighting->pointLights[7].positionAndRadius = glm::vec4{ -arenaRadius, -arenaRadius, 0.0f, arenaLightsRadius };
	lighting->pointLights[8].positionAndRadius = glm::vec4{ 0.0f, -arenaRadius, 0.0f, arenaLightsRadius };
	lighting->pointLights[9].positionAndRadius = glm::vec4{ arenaRadius, -arenaRadius, 0.0f, arenaLightsRadius };

	//ships' lights
	for (int i = 0; i < 2; ++i)
	{
		lighting->pointLights[i].color = glm::vec3{ 1.0f, 0.0f, 0.0f };
		lighting->pointLights[i].attenuation = glm::vec3{ 0.0f, 1.0f, 0.0f }; //linear attenuation
	}

	//arena's lights
	for (int i = 2; i < POINT_LIGHT_COUNT; ++i)
	{
		lighting->pointLights[i].color = glm::vec3{ 1.0f, 1.0f, 0.0f };
		lighting->pointLights[i].attenuation = glm::vec3{0.0f, 1.0f, 0.0f}; //linear attenuation
	}
	
#ifdef FORWARD_RENDER
	forwardRenderer.reset(new ForwardRenderer{});
#else
	deferredRenderer.reset(new DeferredRenderer{});
#endif

	shadowMapRenderer.reset(new ShadowMapRenderer{});
	skyBoxRenderer.reset(new SkyBoxRenderer{});
	skyBoxRenderer->skyBoxMaterial.skyBox = g_textureCubeLibrary.get("SkyBox");	
}

/* method to define stats */

/* TODO:
 *  tune stats more.
 *  make them read from a file!
 * (a step toward moddability!)
 */

void Ship::setStatsAsFighter(){
	stats.turnRate = 104; // deg / s^2
	setMaxVelAndAcc( 30.0f, 60.0f ); // m/s, m/s^2
	stats.fireRate = 8;  // shots per sec
	stats.fireRange = 12.0; // m
	stats.fireSpeed = 35.0; // m/s
}

void Ship::setStatsAsTank(){
	stats.turnRate = 73; // deg / s^2
	setMaxVelAndAcc( 50.0f, 10.0f ); // m/s, m/s^2
	stats.fireRate = 1.3f;  // shots per sec
	stats.fireRange = 52.0f; // m
	stats.fireSpeed = 22.0f; // m/s
}

static void getMaterialAssets(MeshComponent& meshComponent, const std::string& meshName,
							   const std::string& diffuseMapName,
							   const std::string& normalMapName,
							   const std::string& specularMapName)
{
	if (g_meshLibrary.exists(meshName))
	{
		meshComponent.mesh = g_meshLibrary.get(meshName);
	}
	if (g_textureLibrary.exists(diffuseMapName))
	{
		meshComponent.material.diffuseMap = g_textureLibrary.get(diffuseMapName);
	}
	if (g_textureLibrary.exists(normalMapName))
	{
		meshComponent.material.normalMap = g_textureLibrary.get(normalMapName);
	}
	if (g_textureLibrary.exists(specularMapName))
	{
		meshComponent.material.specularMap = g_textureLibrary.get(specularMapName);
	}
}


Ship::Ship()
{
	getMaterialAssets(meshComponent, "ShipMesh", "ShipDiffuseMap", "ShipNormalMap", "ShipSpecularMap");
	
	meshComponent.material.setSpecularExponent(80.0f);
	meshComponent.material.setSpecularColor(glm::vec3{ 1.0f, 1.0f, 1.0f });
}

Bullet::Bullet()
{
	getMaterialAssets(meshComponent, "BulletMesh", "BulletDiffuseMap", "BulletNormalMap", "BulletSpecularMap");

	meshComponent.material.setSpecularExponent(40.0f);
	meshComponent.material.setSpecularColor(glm::vec3{ 0.2f, 0.2f, 0.2f });
}

Floor::Floor()
{
	getMaterialAssets(meshComponent, "FloorMesh", "FloorDiffuseMap", "FloorNormalMap", "FloorSpecularMap");

	meshComponent.material.setTextCoordScale(glm::vec2{ 4.0f, 4.0f });
	meshComponent.material.setSpecularExponent(28.0f);
	meshComponent.material.setSpecularColor(glm::vec3{ 0.2f, 0.2f, 0.2f });
}
