#ifndef __BUMP_MESH_H__
#define __BUMP_MESH_H__

#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#ifdef _WIN32
#include "windows.h"
#endif
#include <GL/gl.h>
#include "load3ds.h"
#include "mathlib.h"

typedef struct {
	vec3_t xyz;
	vec3_t normal;
	vec3_t tangent;
	vec3_t binormal;
	vec3_t basis[3];
	vec2_t st;
} bump_vertex_t;

typedef struct {
	bump_vertex_t *vertex;
	int num_vertex;
} bump_mesh_t;

bump_mesh_t *bump_mesh_load(char *name);
void bump_mesh_render(bump_mesh_t *bm,float *transform);
void bump_mesh_render_light(bump_mesh_t *bm,float *transform,float *light);
int bump_create_cube_map(int size);

#endif /* __BUMP_MESH_H__ */
