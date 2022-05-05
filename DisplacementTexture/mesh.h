/*	mesh
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "mathlib.h"

class Mesh {
public:
	Mesh();
	Mesh(const char *name);
	~Mesh();
	
	void load(const char *name);
	void load3ds(const char *name);
	
	void setTexture(int unit,GLuint texture_id,int s = -1);
	void bindTexture(int s = -1);
	int render(int s = -1,int ppl = 0);
	
	int getStartVertex(int s = -1);
	int getNumVertex(int s = -1);
	int getNumSurface();
	
	int getSurface(const char *name);
	const char *getSurfaceName(int s) const;
	
	float *getVertex(int s = -1);
	float *getNormal(int s = -1);
	float *getTangent(int s = -1);
	float *getBinormal(int s = -1);
	float *getTexCoord(int s = -1);
	
	vec3 getMin(int s = -1);
	vec3 getMax(int s = -1);
	vec3 getCenter(int s = -1);
	float getRadius(int s = -1);
	
protected:
	vec3 *vertex;
	vec3 *normal;
	vec3 *tangent;
	vec3 *binormal;
	vec2 *st;
	int num_vertex;
	
	vec3 min;
	vec3 max;
	vec3 center;
	float radius;
	
	struct Surface {
		char name[256];
		int start_vertex;
		int num_vertex;
		vec3 min;
		vec3 max;
		vec3 center;
		float radius;
		GLuint list_id;
		GLuint texture_id[8];
	};
	
	std::vector<Surface*> surface;
	
	void getFrustum();
	int checkBox(const vec3 &min,const vec3 &max);
	vec4 frustum[6];
};

#endif /* __MESH_H__ */
