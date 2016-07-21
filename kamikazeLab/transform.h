#ifndef TRANSFORM_H
#define TRANSFORM_H

/* Tranforms.h -
 * a basic class for trasnform
 */

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace glm;

struct Transform{

	Transform()
	{
		setIde();
	}

	vec3 pos;  // aka "translation"
	quat ori;  // aka "rotation"
	float scale; // note: Unity uses anisotropic scaling instead (a vec3 for scale)
				 // (which makes transofrms not closed to combination)

	void setIde(){
		pos = vec3(0,0,0);
		ori = quat(1,0,0,0);
		scale = 1;
	}

	vec3 forward() const {
		//return
		// quat::rotate(ori, vec3(0,0,1) );
		quat tmp = (ori * quat(0, 0,1,0) * conjugate(ori));
		return vec3( tmp.x, tmp.y, tmp.z );

	}

	void setModelMatrix(glm::mat4&) const;
	glm::mat4 getModelMatrix()const;

	/*TODO: very good exercises:
	 * methods for:
	 * cumulate, invert, transform points/vectors, interpolate (mix) */

	Transform inverse() const {
		Transform res;
		res.scale = 1.0f/scale;
		res.ori = glm::inverse( ori );
		res.pos = res.ori * -pos;
		return res;
	}

};

#endif // TRANSFORM_H
