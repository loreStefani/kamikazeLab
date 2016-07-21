#ifndef _FORWARD_RENDERER_H_
#define _FORWARD_RENDERER_H_

class PhysObject;

struct ForwardRenderer
{
	void render(PhysObject** physObjects, unsigned int count)const;
private:
	void renderPhysObject(PhysObject& physObject)const;
};

#endif