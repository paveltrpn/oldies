/*	object
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "mathlib.h"
#include "mesh.h"
#include "texture.h"

class Object {
public:
	Object();
	Object(const char *name);
	~Object();
	
	void load(const char *name);
	void render();
	void render_mesh();
	Mesh &getMesh() { return *mesh; }
	
protected:
	enum {
		MAX_SURFACE = 1024,
		MAX_MATERIAL = 1024
	};
	
	struct Material {
		const char *name;
		Texture *diffuse;
		Texture *env;
		int surface[MAX_SURFACE];
		int num_surface;
	};
	Mesh *mesh;
	Material *material[MAX_MATERIAL];
	int num_material;
};

#endif /* __OBJECT_H__ */
