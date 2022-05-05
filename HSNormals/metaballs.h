/*	metaballs
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __METABALLS_H__
#define __METABALLS_H__

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "mathlib.h"

class MetaBalls {
public:
	MetaBalls(float size,float step);
	~MetaBalls();

	int create(vec3 *dest,float (*func)(const vec3 &),float isolevel);

protected:
	int cell(vec3 *dest,const vec3 *xyz,const float *field,float isolevel);
	
	float size;
	float step;
	float *field;

	static int edge[256];
	static int triangles[256][16];
};

#endif /* __METABALLS_H__ */
