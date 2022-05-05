/*  skinned mesh
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#ifndef __SKINNEDMESH_H__
#define __SKINNEDMESH_H__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include "loadtga.h"
#include "loadjpeg.h"
#include "mathlib.h"

#define NAME_LENGTH 64
#define MAX_SURFACES 64

typedef struct {
    char name[NAME_LENGTH];
    float matrix[16];
} bone_t;

typedef struct {
    float xyz[3];
    float rot[4];
} frame_t;

typedef struct {
    int bone;
    float weight;
    float xyz[3];
    float normal[3];
} weight_t;

typedef struct {
    float xyz[3];
    float normal[3];
    float st[2];
    int num_weight;
    weight_t *weight;
} vertex_t;

typedef struct {
    int v0,v1,v2;
} face_t;

typedef struct {
    char name[NAME_LENGTH];
    vertex_t *vertex;
    int num_vertex;
    face_t *face;
    int num_face;
    int textureid;
} surface_t;

typedef struct {
    bone_t *bone;
    int num_bone;
    frame_t **frame;
    int num_frame;
    surface_t *surface[MAX_SURFACES];
    int num_surface;
} skinnedmesh_t;

skinnedmesh_t *skinnedmesh_load_ascii(char *name);
void skinnedmesh_load_skin(skinnedmesh_t *sm,char *name);
void skinnedmesh_frame(skinnedmesh_t *sm,float frame);
void skinnedmesh_render(skinnedmesh_t *sm);

#endif /* __SKINNEDMESH_H__ */
