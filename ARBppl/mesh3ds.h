/*  mesh3ds
 *	loading and save 3ds file
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 */

#ifndef __MESH3DS_H__
#define __MESH3DS_H__

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

/* struct {
 *		float xyz[3];
 *		float normal[3];
 *		float st[2];
 * } vertex;
 */

float *mesh3ds_load(char *name,int *num_vertex);
int mesh3ds_save(char *name,float *vertex,int num_vertex);
void mesh3ds_render(float *vertex,int num_vertex);
int mesh3ds_gen_list(float *vertex,int num_vertex);

#endif /* __MESH3DS_H__ */
