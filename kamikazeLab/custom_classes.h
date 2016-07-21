#ifndef SCENE_H
#define SCENE_H

/* custom_classes.h:
 *
 * these are ad-hoc structures used by this specific game:
 *  - Stats, (of a ship)
 *  - Ship,
 *  - Bullet,
 *  - Scene (all data of a game, also the arena)
 *
 * (in class, this file was called "scene.h")
 *
 * Often, they inhert from general "PhysObjects" and add fields and/or
 * add / redefine funcionalies. They use methods of their base class.
 *
 */
#include <vector>
#include "phys_object.h"
#include "controller.h"
#include <memory>

struct Stats{
	float accRate;
	float turnRate;
	float fireRate; // bullets per seconds
	float fireRange;
	float fireSpeed;
};

struct Bullet : public PhysObject {

	Bullet();

	float timeToLive;
	bool alive;
	void doPhysStep();
	void reset(){ alive = false; }
};


struct Ship: public PhysObject{

	Ship();

	Stats stats;

	ShipController controller;

	void doPhysStep();

	void setMaxVelAndAcc( float maxVel, float acc );
	std::vector< Bullet > bullets;
	float timeBeforeFiringAgain;
	void spawnNewBullet();

	Bullet& findUnusedBullet();
	void fillBullet(Bullet& b) const;

	void reset();
	void die();
	void respawn();

	void setStatsAsFighter(); //
	void setStatsAsTank();
	bool alive;
	double timeDead;	
};

struct Floor : PhysObject
{
	Floor();
};

struct Camera
{
	mat4 viewTransform;
	mat4 projectionTransform;
	mat4 projectionViewTransform;
	mat4 invViewTransform;
	mat4 invProjectionTransform;
	Transform transform;
	float nearPlane;
	float farPlane;
	float fovY;
	float aspectRatio;

	void computeViewFromTransform();
	void computeInvView();
	void computeInvProj();
	void computeProjectionView();
	void setProjectionParams(float fovY, float aspectRatio, float near, float far);
};

struct SceneLighting;
struct DeferredRenderer;
struct ForwardRenderer;
struct ShadowMapRenderer;
struct SkyBoxRenderer;

struct Scene{

	float arenaRadius;
	std::vector< Ship > ships;

	vec3 randomPosInArena() const;
	void initAsNewGame();
	void render();

	void doPhysStep();

	bool isInside( vec3 p ) const;
	vec3 pacmanWarp( vec3 p) const;
		
	Camera camera;

	std::unique_ptr<SceneLighting> lighting;
	std::unique_ptr<Floor> floor;
	
	std::unique_ptr<DeferredRenderer> deferredRenderer;
	std::unique_ptr<ForwardRenderer> forwardRenderer;
	std::unique_ptr<ShadowMapRenderer> shadowMapRenderer;
	std::unique_ptr<SkyBoxRenderer> skyBoxRenderer;

private:
	void checkAllCollisions();
	glm::mat4 cameraOnTwoObjects(const PhysObject& a, const PhysObject& b);
	void findVisiblePhysObjects();
	std::vector<PhysObject*> physObjects;
};

extern Scene scene; // a poor man's singleton (there is one, and everyone can use it)

#endif // SCENE_H
