/*	mesh
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef __MESH_H__
#define __MESH_H__

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "mathlib.h"

class Mesh {
public:
	Mesh();
	Mesh(const char *name);
	~Mesh();
	
	void load(const char *name);
	void load3ds(const char *name);
	void render();
	void render(int s);
	
	int getNumVertex();
	float *getVertex();
	float *getNormal();
	float *getTangent();
	float *getBinormal();
	float *getTexCoord();
	
	int getNumSurface();
	int getSurface(const char *name);
	const char *getSurfaceName(int s) const;
	int getStartVertex(int s);
	int getNumVertex(int s);
	float *getVertex(int s);
	float *getNormal(int s);
	float *getTangent(int s);
	float *getBinormal(int s);
	float *getTexCoord(int s);

protected:
	vec3 *vertex;
	vec3 *normal;
	vec3 *tangent;
	vec3 *binormal;
	vec2 *st;
	int num_vertex;
	
	enum {
		MAX_SURFACE = 1024
	};
	
	struct Surface {
		char name[256];
		int start_vertex;
		int num_vertex;
	};

	Surface *surface[MAX_SURFACE];
	int num_surface;
};

#endif /* __MESH_H__ */
