/* Mesh
 *
 * Copyright (C) 2003-2005, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>

#include "GLExt.h"
#include "Mesh.h"

/*****************************************************************************/
/*                                                                           */
/* Mesh                                                                      */
/*                                                                           */
/*****************************************************************************/

/*
 */
Mesh::Mesh() {
	min = vec3(100000.0f,1000000.0f,1000000.0f);
	max = vec3(-1000000.0f,-1000000.0f,-1000000.0f);
	center = vec3(0,0,0);
	radius = 1000000.0f;
}

Mesh::Mesh(const char *name) {
	load(name);
}

/*
 */
Mesh::~Mesh() {
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s = surfaces[i];
		delete [] s->vertex;
		delete [] s->cvertex;
		delete [] s->edges;
		delete [] s->triangles;
		if(s->vertex_vbo_id) glDeleteBuffersARB(1,&s->vertex_vbo_id);
		if(s->indices_vbo_id) glDeleteBuffersARB(1,&s->indices_vbo_id);
		if(s->vertex_clip_vbo_id) glDeleteBuffersARB(1,&s->vertex_clip_vbo_id);
		if(s->shadow_volume_vertex_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_vertex_vbo_id);
		if(s->shadow_volume_indices_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_indices_vbo_id);
		if(s->shadow_volume_caps_indices_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_caps_indices_vbo_id);
		delete s;
	}
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::enable() {
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glEnableVertexAttribArrayARB(4);
}

/*
 */
void Mesh::disable() {
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(4);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
}

/*
 */
void Mesh::bind(int surface) {
	
	Surface *s = surfaces[surface];
	if(s->vertex_vbo_id == 0) create_vbo(s);
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_vbo_id);
	glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),0);
	glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
	glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 2));
	glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 3));
	glVertexAttribPointerARB(4,4,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 4));	
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->indices_vbo_id);
}

/*
 */
int Mesh::render(int surface) {
	Surface *s = surfaces[surface];
	glDrawElements(GL_TRIANGLES,s->num_triangles * 3,GL_UNSIGNED_INT,0);
	return s->num_triangles;
}

/*
 */
int Mesh::render() {
	int num_triangles = 0;
	enable();
	for(int i = 0; i < surfaces.size(); i++) {
		bind(i);
		num_triangles += render(i);
	}
	disable();
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* vertex clip                                                               */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::enableVertexClip() {
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glEnableVertexAttribArrayARB(4);
	glEnableVertexAttribArrayARB(5);
	glEnableVertexAttribArrayARB(6);
}

/*
 */
void Mesh::disableVertexClip() {
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(6);
	glDisableVertexAttribArrayARB(5);
	glDisableVertexAttribArrayARB(4);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
}

/*
 */
void Mesh::bindVertexClip(int surface) {
	
	Surface *s = surfaces[surface];
	if(s->vertex_clip_vbo_id == 0) create_vertex_clip_vbo(s);
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_clip_vbo_id);
	glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(VertexClip),0);
	glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 1));
	glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 2));
	glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 3));
	glVertexAttribPointerARB(4,4,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 4));
	glVertexAttribPointerARB(5,4,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 4 + sizeof(vec4)));
	glVertexAttribPointerARB(6,4,GL_FLOAT,0,sizeof(VertexClip),(void*)(sizeof(vec3) * 4 + sizeof(vec4) * 2));
}

/*
 */
int Mesh::renderVertexClip(int surface) {
	Surface *s = surfaces[surface];
	glDrawArrays(GL_TRIANGLES,0,s->num_triangles * 3);
	return s->num_triangles;
}

/*
 */
int Mesh::renderVertexClip() {
	int num_triangles = 0;
	enableVertexClip();
	for(int i = 0; i < surfaces.size(); i++) {
		bindVertexClip(i);
		num_triangles += renderVertexClip(i);
	}
	disableVertexClip();
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* shadow volumes                                                            */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::getNumIntersections(const vec3 &light,const vec3 &camera,float delta,int surface) {
	
	int num_intersections = 0;
	vec3 dir = camera - light;
	
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		float dot = (dir * s->center - dir * light) / (dir * dir);
		if(dot < 0.0 && length(light - s->center) > s->radius + delta) continue;
		else if(dot > 1.0 && length(camera - s->center) > s->radius + delta) continue;
		else if(length(light + dir * dot - s->center) > s->radius + delta) continue;
		
		dot_vec4_vec4(vec4(light,1.0),s->triangles->plane,sizeof(Triangle),&s->triangles->dot,sizeof(Triangle),s->num_triangles);
		
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			if(t->dot < 0.0) continue;
			float dot = -(t->plane * light) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = light + dir * dot;
			if(p * t->c[0] > -delta && p * t->c[1] > -delta && p * t->c[2] > -delta) num_intersections++;
		}
	}
	
	return num_intersections;
}

/*
 */
void Mesh::enableShadowVolume() {
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
}

/*
 */
void Mesh::disableShadowVolume() {
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
}

/*
 */
void Mesh::bindShadowVolume(int surface) {
	
	Surface *s = surfaces[surface];
	if(s->shadow_volume_vertex_vbo_id == 0) create_shadow_volume_vbo(s);
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->shadow_volume_vertex_vbo_id);
	glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(VertexShadowVolume),0);
	glVertexAttribPointerARB(1,4,GL_FLOAT,0,sizeof(VertexShadowVolume),(void*)(sizeof(vec4)));
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->shadow_volume_indices_vbo_id);
}

/*
 */
void Mesh::bindShadowVolumeCaps(int surface) {
	
	Surface *s = surfaces[surface];
	if(s->shadow_volume_vertex_vbo_id == 0) create_shadow_volume_vbo(s);
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->shadow_volume_vertex_vbo_id);
	glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(VertexShadowVolume),0);
	glVertexAttribPointerARB(1,4,GL_FLOAT,0,sizeof(VertexShadowVolume),(void*)(sizeof(vec4)));
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->shadow_volume_caps_indices_vbo_id);
}

/*
 */
int Mesh::renderShadowVolume(int surface) {
	Surface *s = surfaces[surface];
	glDrawElements(GL_TRIANGLES,s->shadow_volume_num_indices,GL_UNSIGNED_INT,0);
	return s->shadow_volume_num_indices / 3;
}

/*
 */
int Mesh::renderShadowVolumeCaps(int surface) {
	Surface *s = surfaces[surface];
	glDrawElements(GL_TRIANGLES,s->shadow_volume_num_caps_indices,GL_UNSIGNED_INT,0);
	return s->shadow_volume_num_caps_indices / 3;
}

/*
 */
int Mesh::renderShadowVolume() {
	int num_triangles = 0;
	enableShadowVolume();
	for(int i = 0; i < surfaces.size(); i++) {
		bindShadowVolume(i);
		num_triangles += renderShadowVolume(i);
	}
	disableShadowVolume();
	return num_triangles;
}

/*
 */
int Mesh::renderShadowVolumeCaps() {
	int num_triangles = 0;
	enableShadowVolume();
	for(int i = 0; i < surfaces.size(); i++) {
		bindShadowVolumeCaps(i);
		num_triangles += renderShadowVolumeCaps(i);
	}
	disableShadowVolume();
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* line inersection                                                          */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::intersection(const vec3 &p0,const vec3 &p1,vec3 &ret_point,vec3 &ret_normal,int surface) {
	int ret_surface,ret_triangle;
	return intersection(p0,p1,ret_point,ret_normal,ret_surface,ret_triangle,surface);
}

/*
 */
int Mesh::intersection(const vec3 &p0,const vec3 &p1,vec3 &ret_point,vec3 &ret_normal,int &ret_surface,int &ret_triangle,int surface) {
	
	float nearest = 1.0f;
	
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		vec3 dir = p1 - p0;
		
		float dot = (dir * s->center - dir * p0) / (dir * dir);
		if(dot < 0.0 && length(p0 - s->center) > s->radius) continue;
		else if(dot > 1.0 && length(p1 - s->center) > s->radius) continue;
		else if(length(p0 + dir * dot - s->center) > s->radius) continue;
		
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			float dot = -(t->plane * p0) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = p0 + dir * dot;
			if(nearest > dot && p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) {
				nearest = dot;
				ret_point = p;
				ret_normal = t->plane;
				ret_surface = i;
				ret_triangle = j;
			}
		}
	}
	
	if(surface != -1) ret_surface = surface;
	
	return nearest < 1.0f ? 1 : 0;
}

/*****************************************************************************/
/*                                                                           */
/* io                                                                        */
/*                                                                           */
/*****************************************************************************/

/* load
 */
int Mesh::load(const char *name) {
	if(strstr(name,".mesh") || strstr(name,".MESH")) return load_mesh(name);
	if(strstr(name,".3ds") || strstr(name,".3DS")) return load_3ds(name);
	fprintf(stderr,"Mesh::load(): unknown format of \"%s\" file\n",name);
	return 0;
}

/* save
 */
int Mesh::save(const char *name) {
	if(strstr(name,".mesh") || strstr(name,".MESH")) return save_mesh(name);
	fprintf(stderr,"Mesh::save(): unknown format of \"%s\" file\n",name);
	return 0;
}

/* add surface
 */
void Mesh::addSurface(Mesh *mesh,int surface) {
	int num_vertex;
	Vertex *vertex = mesh->getSurface(surface,num_vertex);
	
	addSurface(mesh->getSurfaceName(surface),vertex,num_vertex);
	
	delete [] vertex;
}

/* add surface
 */
void Mesh::addSurface(const char *name,Vertex *vertex,int num_vertex) {
	
	Surface *s = new Surface;
	memset(s,0,sizeof(Surface));
	
	strcpy(s->name,name);
	
	s->num_vertex = num_vertex;
	s->vertex = new Vertex[s->num_vertex];
	memcpy(s->vertex,vertex,sizeof(Vertex) * s->num_vertex);
	
	create_surface(s);
}

/* get surface (tree vertex per triangle)
 */
Mesh::Vertex *Mesh::getSurface(int surface,int &num_vertex) {
	
	Surface *s = surfaces[surface];
	
	num_vertex = s->num_triangles * 3;
	
	Vertex *vertex = new Vertex[num_vertex];
	for(int i = 0; i < s->num_triangles; i++) {
		Triangle *t = &s->triangles[i];
		vertex[i * 3 + 0] = s->vertex[t->v[0]];
		vertex[i * 3 + 1] = s->vertex[t->v[1]];
		vertex[i * 3 + 2] = s->vertex[t->v[2]];
	}
	
	return vertex;
}

/* find surface
 */
int Mesh::findSurface(const char *name) {
	if(name == NULL) return -1;
	for(int i = 0; i < surfaces.size(); i++) {
		if(!strcmp(name,surfaces[i]->name)) return i;
	}
	return -1;
}

/* transformation
 */
void Mesh::setTransform(const mat4 &m,int surface) {
	
	mat4 r = m.rotation();
	
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		for(int j = 0; j < s->num_vertex; j++) {
			Vertex *v = &s->vertex[j];
			v->xyz = m * v->xyz;
			v->normal = r * v->normal;
			v->normal.normalize();
			v->tangent = r * v->tangent;
			v->tangent.normalize();
			v->binormal = r * v->binormal;
			v->binormal.normalize();
		}
		
		for(int j = 0; j < s->num_cvertex; j++) {
			s->cvertex[j] = m * s->cvertex[j];
		}
		
		for(int i = 0; i < s->num_triangles; i++) {
			Triangle *t = &s->triangles[i];
			vec3 normal;
			
			normal = normalize(cross(s->cvertex[t->cv[1]] - s->cvertex[t->cv[0]],s->cvertex[t->cv[2]] - s->cvertex[t->cv[0]]));
			t->plane = vec4(normal,-s->cvertex[t->cv[0]] * normal);
			normal = normalize(cross(t->plane,s->cvertex[t->cv[0]] - s->cvertex[t->cv[2]]));
			t->c[0] = vec4(normal,-s->cvertex[t->cv[0]] * normal);
			normal = normalize(cross(t->plane,s->cvertex[t->cv[1]] - s->cvertex[t->cv[0]]));
			t->c[1] = vec4(normal,-s->cvertex[t->cv[1]] * normal);
			normal = normalize(cross(t->plane,s->cvertex[t->cv[2]] - s->cvertex[t->cv[1]]));
			t->c[2] = vec4(normal,-s->cvertex[t->cv[2]] * normal);
		}
		
		// update buffer objects
		if(s->vertex_vbo_id || s->vertex_clip_vbo_id || s->shadow_volume_vertex_vbo_id) create_vbo(s);

		// recalculate bounds
		calculate_bounds(s);
	}
}

/*****************************************************************************/
/*                                                                           */
/* calculate tangent                                                         */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::calculate_tangent(Surface *s) {
	
	for(int i = 0; i < s->num_vertex; i++) {
		Vertex *v = &s->vertex[i];
		v->tangent = vec3(0,0,0);
		v->binormal = vec3(0,0,0);
	}
	
	for(int i = 0; i < s->num_triangles; i++) {
		Triangle *t = &s->triangles[i];
		
		Vertex *v0 = &s->vertex[t->v[0]];
		Vertex *v1 = &s->vertex[t->v[1]];
		Vertex *v2 = &s->vertex[t->v[2]];
		
		vec3 tangent,binormal;
		vec3 e0 = vec3(0,v1->texcoord.x - v0->texcoord.x,v1->texcoord.y - v0->texcoord.y);
		vec3 e1 = vec3(0,v2->texcoord.x - v0->texcoord.x,v2->texcoord.y - v0->texcoord.y);
		
		for(int j = 0; j < 3; j++) {
			e0[0] = v1->xyz[j] - v0->xyz[j];
			e1[0] = v2->xyz[j] - v0->xyz[j];
			vec3 v = cross(e0,e1);
			if(fabs(v[0]) > EPSILON) {
				tangent[j] = -v[1] / v[0];
				binormal[j] = -v[2] / v[0];
			} else {
				tangent[j] = j;
				binormal[j] = j + 1;
			}
		}
		
		tangent = normalize(tangent);
		binormal = normalize(binormal);
		
		v0->tangent += tangent;
		v0->binormal += binormal;
		v1->tangent += tangent;
		v1->binormal += binormal;
		v2->tangent += tangent;
		v2->binormal += binormal;
	}
	
	for(int i = 0; i < s->num_vertex; i++) {
		Vertex *v = &s->vertex[i];
		if(length(v->tangent) < EPSILON || length(v->binormal) < EPSILON) {
			if(length(v->tangent) > EPSILON) {
				v->tangent.normalize();
				v->binormal = normalize(cross(v->normal,v->tangent));
			} else if(length(v->binormal) > EPSILON) {
				v->binormal.normalize();
				v->tangent = normalize(cross(v->binormal,v->normal));
			} else {
				if(fabs(v->normal.z) < 0.5f) v->tangent = vec3(0,0,1);
				else v->tangent = vec3(1,0,0);
				v->binormal = normalize(cross(v->normal,v->tangent));
				v->tangent = normalize(cross(v->binormal,v->normal));
			}
		} else {
			v->tangent.normalize();
			v->binormal.normalize();
		}
		if(cross(v->tangent,v->binormal) * v->normal < 0.0f) {
			v->binormal = normalize(cross(v->tangent,v->normal));
			v->tangent = -normalize(cross(v->binormal,v->normal));
		} else {
			v->binormal = normalize(cross(v->normal,v->tangent));
			v->tangent = normalize(cross(v->binormal,v->normal));
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* calculate bounds                                                          */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::calculate_bounds(Surface *s) {
	
	// calculale bounds for the surface
	s->min = vec3(1000000.0f,1000000.0f,1000000.0f);
	s->max = vec3(-1000000.0f,-1000000.0f,-1000000.0f);
	for(int i = 0; i < s->num_cvertex; i++) {
		if(s->max.x < s->cvertex[i].x) s->max.x = s->cvertex[i].x;
		if(s->min.x > s->cvertex[i].x) s->min.x = s->cvertex[i].x;
		if(s->max.y < s->cvertex[i].y) s->max.y = s->cvertex[i].y;
		if(s->min.y > s->cvertex[i].y) s->min.y = s->cvertex[i].y;
		if(s->max.z < s->cvertex[i].z) s->max.z = s->cvertex[i].z;
		if(s->min.z > s->cvertex[i].z) s->min.z = s->cvertex[i].z;
	}
	s->center = (s->min + s->max) / 2.0f;
	s->radius = 0.0;
	for(int i = 0; i < s->num_cvertex; i++) {
		float r = length(s->center - s->cvertex[i]);
		if(r > s->radius) s->radius = r;
	}
	
	// recalculate bounds for the mesh
	min = vec3(1000000.0f,1000000.0f,1000000.0f);
	max = vec3(-1000000.0f,-1000000.0f,-1000000.0f);
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s = surfaces[i];
		if(max.x < s->max.x) max.x = s->max.x;
		if(min.x > s->min.x) min.x = s->min.x;
		if(max.y < s->max.y) max.y = s->max.y;
		if(min.y > s->min.y) min.y = s->min.y;
		if(max.z < s->max.z) max.z = s->max.z;
		if(min.z > s->min.z) min.z = s->min.z;
	}
	center = (min + max) / 2.0f;
	radius = 0.0;
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s = surfaces[i];
		for(int j = 0; j < s->num_cvertex; j++) {
			float r = length(center - s->cvertex[j]);
			if(r > radius) radius = r;
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* create vertex buffer objects                                              */
/*                                                                           */
/*****************************************************************************/

/*
 */
struct cvbo_VertexShadowVolume {
	Mesh::VertexShadowVolume vertex;
	int id;
};

/*
 */
static int cvbo_svvertex_cmp(const void *a,const void *b) {
	cvbo_VertexShadowVolume *cv_svv0 = (cvbo_VertexShadowVolume*)a;
	cvbo_VertexShadowVolume *cv_svv1 = (cvbo_VertexShadowVolume*)b;
	Mesh::VertexShadowVolume *v0 = &cv_svv0->vertex;
	Mesh::VertexShadowVolume *v1 = &cv_svv1->vertex;
	float d;
	d = v0->xyz[0] - v1->xyz[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->xyz[1] - v1->xyz[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->xyz[2] - v1->xyz[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->xyz[3] - v1->xyz[3];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->plane[0] - v1->plane[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->plane[1] - v1->plane[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->plane[2] - v1->plane[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->plane[3] - v1->plane[3];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	return 0;
}

/*
 */
void Mesh::create_vbo(Surface *s) {
	
	if(s->vertex_vbo_id) glDeleteBuffersARB(1,&s->vertex_vbo_id);
	if(s->indices_vbo_id) glDeleteBuffersARB(1,&s->indices_vbo_id);
	
	glGenBuffersARB(1,&s->vertex_vbo_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_vbo_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(Vertex) * s->num_vertex,s->vertex,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	int *indices = new int[s->num_triangles * 3];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		indices[j + 0] = t->v[0];
		indices[j + 1] = t->v[1];
		indices[j + 2] = t->v[2];
	}
	
	glGenBuffersARB(1,&s->indices_vbo_id);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->indices_vbo_id);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,sizeof(int) * s->num_triangles * 3,indices,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	delete [] indices;
}

/*
 */
void Mesh::create_vertex_clip_vbo(Surface *s) {
	
	if(s->vertex_clip_vbo_id) glDeleteBuffersARB(1,&s->vertex_clip_vbo_id);
	
	VertexClip *vertex_clip = new VertexClip[s->num_triangles * 3];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		
		for(int k = 0; k < 3; k++) {
			vertex_clip[j + k].vertex.xyz = s->vertex[t->v[k]].xyz;
			vertex_clip[j + k].vertex.normal = s->vertex[t->v[k]].normal;
			vertex_clip[j + k].vertex.tangent = s->vertex[t->v[k]].tangent;
			vertex_clip[j + k].vertex.binormal = s->vertex[t->v[k]].binormal;
			vertex_clip[j + k].vertex.texcoord = s->vertex[t->v[k]].texcoord;
		}
		
		vertex_clip[j + 0].plane = t->plane;
		vertex_clip[j + 1].plane = t->plane;
		vertex_clip[j + 2].plane = t->plane;
		
		vec3 min = vec3(1000000.0f,1000000.0f,1000000.0f);
		vec3 max = vec3(-1000000.0f,-1000000.0f,-1000000.0f);
		for(int k = 0; k < 3; k++) {
			if(min.x > s->vertex[t->v[k]].xyz.x) min.x = s->vertex[t->v[k]].xyz.x;
			if(max.x < s->vertex[t->v[k]].xyz.x) max.x = s->vertex[t->v[k]].xyz.x;
			if(min.y > s->vertex[t->v[k]].xyz.y) min.y = s->vertex[t->v[k]].xyz.y;
			if(max.y < s->vertex[t->v[k]].xyz.y) max.y = s->vertex[t->v[k]].xyz.y;
			if(min.z > s->vertex[t->v[k]].xyz.z) min.z = s->vertex[t->v[k]].xyz.z;
			if(max.z < s->vertex[t->v[k]].xyz.z) max.z = s->vertex[t->v[k]].xyz.z;
		}
		
		vec3 center = (min + max) / 2.0f;
		float radius = 0;
		for(int k = 0; k < 3; k++) {
			float r = length(s->vertex[t->v[k]].xyz - center);
			if(radius < r) radius = r;
		}
		
		vertex_clip[j + 0].center = vec4(center,radius);
		vertex_clip[j + 1].center = vec4(center,radius);
		vertex_clip[j + 2].center = vec4(center,radius);
	}
	
	glGenBuffersARB(1,&s->vertex_clip_vbo_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_clip_vbo_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(VertexClip) * s->num_triangles * 3,vertex_clip,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	delete [] vertex_clip;
}

/*
 */
void Mesh::create_shadow_volume_vbo(Surface *s) {
	
	if(s->shadow_volume_vertex_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_vertex_vbo_id);
	if(s->shadow_volume_indices_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_indices_vbo_id);
	if(s->shadow_volume_caps_indices_vbo_id) glDeleteBuffersARB(1,&s->shadow_volume_caps_indices_vbo_id);
	
	struct cvbo_Edge {
		int num_triangles;	// number of triangles per edge
		int t[4];			// triangles numbers
		int v0[4];			// triangles vertexes
		int v1[4];
	};
	
	cvbo_Edge *edges = new cvbo_Edge[s->num_edges];
	
	// fill edges
	for(int i = 0; i < s->num_edges; i++) edges[i].num_triangles = 0;
	for(int i = 0; i < s->num_triangles; i++) {
		Triangle *t = &s->triangles[i];
		if(t->cv[0] == t->cv[1] || t->cv[1] == t->cv[2] || t->cv[2] == t->cv[0]) continue;
		for(int j = 0; j < 3; j++) {
			cvbo_Edge *e = &edges[t->e[j]];
			if(e->num_triangles == sizeof(e->t) / sizeof(e->t[0])) {
				fprintf(stderr,"Mesh::create_vbo(): too many triangles per edge\n");
				continue;
			}
			e->t[e->num_triangles] = i;
			e->v0[e->num_triangles] = (j + 1) % 3;
			e->v1[e->num_triangles] = (j + 2) % 3;
			e->num_triangles++;
		}
	}
	
	/////////////////////////////////
	// create shadow volume vertexes
	/////////////////////////////////
	
	// new number of vertexes
	int num_vertex = s->num_triangles * 3;
	for(int i = 0; i < s->num_edges; i++) {
		cvbo_Edge *e = &edges[i];
		if(e->num_triangles == 1) num_vertex += 4;
	}
	
	// create shadow volume vertexes
	VertexShadowVolume *vertex = new VertexShadowVolume[num_vertex];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		for(int k = 0; k < 3; k++) {
			vertex[j + k].xyz = vec4(s->cvertex[t->cv[k]],0.0f);
			vertex[j + k].plane = t->plane;
		}
	}
	// add new vertexes for edges which has one triangle
	for(int i = 0, j = s->num_triangles * 3; i < s->num_edges; i++) {
		cvbo_Edge *e = &edges[i];
		if(e->num_triangles == 1) {
			vertex[j + 0].xyz = vec4(s->cvertex[s->triangles[e->t[0]].cv[e->v0[0]]],0.0f);
			vertex[j + 0].plane = s->triangles[e->t[0]].plane;
			vertex[j + 1].xyz = vec4(s->cvertex[s->triangles[e->t[0]].cv[e->v1[0]]],0.0f);
			vertex[j + 1].plane = s->triangles[e->t[0]].plane;
			vertex[j + 2].xyz = vec4(s->cvertex[s->triangles[e->t[0]].cv[e->v1[0]]],1.0f);
			vertex[j + 2].plane = s->triangles[e->t[0]].plane;
			vertex[j + 3].xyz = vec4(s->cvertex[s->triangles[e->t[0]].cv[e->v0[0]]],1.0f);
			vertex[j + 3].plane = s->triangles[e->t[0]].plane;
			j += 4;
		}
	}
	
	/////////////////////////////////
	// optimize shadow volume vertexes
	/////////////////////////////////
	
	cvbo_VertexShadowVolume *v = new cvbo_VertexShadowVolume[num_vertex];
	for(int i = 0; i < num_vertex; i++) {
		v[i].vertex = vertex[i];
		v[i].id = i;
	}
	
	qsort(v,num_vertex,sizeof(cvbo_VertexShadowVolume),cvbo_svvertex_cmp);
	
	int num_optimized_vertex = 0;
	int *vbuf = new int[num_vertex];
	for(int i = 0; i < num_vertex; i++) {
		if(i == 0 || cvbo_svvertex_cmp(&v[num_optimized_vertex - 1],&v[i])) v[num_optimized_vertex++] = v[i];
		vbuf[v[i].id] = num_optimized_vertex - 1;
	}
	
	// copy optimized vertexes
	num_vertex = num_optimized_vertex;
	for(int i = 0; i < num_vertex; i++) {
		vertex[i] = v[i].vertex;
	}
	
	glGenBuffersARB(1,&s->shadow_volume_vertex_vbo_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->shadow_volume_vertex_vbo_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(VertexShadowVolume) * num_vertex,vertex,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	delete [] v;
	delete [] vertex;
	
	/////////////////////////////////
	// create shadow volume indices
	/////////////////////////////////
	
	s->shadow_volume_num_indices = 0;
	for(int i = 0; i < s->num_edges; i++) {
		cvbo_Edge *e = &edges[i];
		if(e->num_triangles == 1) {
			s->shadow_volume_num_indices += 6;
		} else {
			for(int j = 0; j < e->num_triangles; j++) {
				s->shadow_volume_num_indices += j * 6;
			}
		}
	}
	
	int *indices = new int[s->shadow_volume_num_indices];
	
	s->shadow_volume_num_indices = 0;
	for(int i = 0, j = 0, k = s->num_triangles * 3; i < s->num_edges; i++) {
		cvbo_Edge *e = &edges[i];
		if(e->num_triangles == 1) {
			indices[j++] = vbuf[k + 3];
			indices[j++] = vbuf[k + 2];
			indices[j++] = vbuf[k + 1];
			indices[j++] = vbuf[k + 1];
			indices[j++] = vbuf[k + 0];
			indices[j++] = vbuf[k + 3];
			s->shadow_volume_num_indices += 6;
			k += 4;
		} else {
			for(int l = 0; l < e->num_triangles; l++) {
				for(int m = l + 1; m < e->num_triangles; m++) {
					int v0,v1,v2,v3;
					v0 = vbuf[e->t[m] * 3 + e->v1[m]];
					v1 = vbuf[e->t[m] * 3 + e->v0[m]];
					if(s->triangles[e->t[m]].cv[e->v0[m]] == s->triangles[e->t[l]].cv[e->v0[l]]) {
						v2 = vbuf[e->t[l] * 3 + e->v0[l]];
						v3 = vbuf[e->t[l] * 3 + e->v1[l]];
					} else {
						v2 = vbuf[e->t[l] * 3 + e->v1[l]];
						v3 = vbuf[e->t[l] * 3 + e->v0[l]];
					}
					indices[j++] = v0;
					indices[j++] = v1;
					indices[j++] = v2;
					indices[j++] = v2;
					indices[j++] = v3;
					indices[j++] = v0;
					s->shadow_volume_num_indices += 6;
				}
			}
		}
	}
	
	glGenBuffersARB(1,&s->shadow_volume_indices_vbo_id);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->shadow_volume_indices_vbo_id);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,sizeof(int) * s->shadow_volume_num_indices,indices,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	delete [] indices;
	
	/////////////////////////////////
	// create shadow volume caps indices
	/////////////////////////////////
	
	// number of indices
	s->shadow_volume_num_caps_indices = s->num_triangles * 3;
	indices = new int[s->shadow_volume_num_caps_indices];
	
	// create indices
	for(int i = 0; i < s->num_triangles; i++) {
		int j = i * 3;
		indices[j + 0] = vbuf[j + 0];
		indices[j + 1] = vbuf[j + 1];
		indices[j + 2] = vbuf[j + 2];
	}
	
	glGenBuffersARB(1,&s->shadow_volume_caps_indices_vbo_id);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->shadow_volume_caps_indices_vbo_id);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,sizeof(int) * s->shadow_volume_num_caps_indices,indices,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	delete [] indices;
	delete [] vbuf;
	delete [] edges;
}

/*****************************************************************************/
/*                                                                           */
/* create surface                                                            */
/*                                                                           */
/*****************************************************************************/

/*
 */
struct cs_Vertex {
	Mesh::Vertex vertex;
	int id;
};

struct cs_CVertex {
	vec3 vertex;
	int id;
};

struct cs_Edge {
	int v[2];
	int face;
	int num;
	int id;
};

/*
 */
static int cs_vertex_cmp(const void *a,const void *b) {
	cs_Vertex *cs_v0 = (cs_Vertex*)a;
	cs_Vertex *cs_v1 = (cs_Vertex*)b;
	Mesh::Vertex *v0 = &cs_v0->vertex;
	Mesh::Vertex *v1 = &cs_v1->vertex;
	float d;
	d = v0->xyz[0] - v1->xyz[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->xyz[1] - v1->xyz[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->xyz[2] - v1->xyz[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->normal[0] - v1->normal[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->normal[1] - v1->normal[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->normal[2] - v1->normal[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->texcoord[0] - v1->texcoord[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->texcoord[1] - v1->texcoord[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->texcoord[2] - v1->texcoord[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0->texcoord[3] - v1->texcoord[3];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	return 0;
}

static int cs_cvertex_cmp(const void *a,const void *b) {
	cs_CVertex *cs_cv0 = (cs_CVertex*)a;
	cs_CVertex *cs_cv1 = (cs_CVertex*)b;
	vec3 &v0 = cs_cv0->vertex;
	vec3 &v1 = cs_cv1->vertex;
	float d;
	d = v0[0] - v1[0];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0[1] - v1[1];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	d = v0[2] - v1[2];
	if(d > EPSILON) return 1; if(d < -EPSILON) return -1;
	return 0;
}

static int cs_edge_cmp(const void *a,const void *b) {
	cs_Edge *e0 = (cs_Edge*)a;
	cs_Edge *e1 = (cs_Edge*)b;
	int v0[2],v1[2];
	if(e0->v[0] < e0->v[1]) { v0[0] = e0->v[0]; v0[1] = e0->v[1]; }
	else { v0[0] = e0->v[1]; v0[1] = e0->v[0]; }
	if(e1->v[0] < e1->v[1]) { v1[0] = e1->v[0]; v1[1] = e1->v[1]; }
	else { v1[0] = e1->v[1]; v1[1] = e1->v[0]; }
	if(v0[0] > v1[0]) return 1;
	if(v0[0] < v1[0]) return -1;
	if(v0[1] > v1[1]) return 1;
	if(v0[1] < v1[1]) return -1;
	return 0;
}

/*
 */
void Mesh::create_surface(Surface *s) {
	
	// default initialization
	
	s->num_cvertex = s->num_vertex;
	s->num_edges = s->num_vertex;
	s->num_triangles = s->num_vertex / 3;
	
	s->vertex_vbo_id = 0;
	s->indices_vbo_id = 0;
	s->vertex_clip_vbo_id = 0;
	
	s->shadow_volume_vertex_vbo_id = 0;
	s->shadow_volume_num_indices = 0;
	s->shadow_volume_indices_vbo_id = 0;
	s->shadow_volume_num_caps_indices = 0;
	s->shadow_volume_caps_indices_vbo_id = 0;
	
	/////////////////////////////////
	// optimize vertexes
	/////////////////////////////////
	
	cs_Vertex *v = new cs_Vertex[s->num_vertex];
	for(int i = 0; i < s->num_vertex; i++) {
		v[i].vertex = s->vertex[i];
		v[i].id = i;
	}
	
	qsort(v,s->num_vertex,sizeof(cs_Vertex),cs_vertex_cmp);
	
	int num_optimized_vertex = 0;
	int *vbuf = new int[s->num_vertex];
	for(int i = 0; i < s->num_vertex; i++) {
		if(i == 0 || cs_vertex_cmp(&v[num_optimized_vertex - 1],&v[i])) v[num_optimized_vertex++] = v[i];
		vbuf[v[i].id] = num_optimized_vertex - 1;
	}
	
	/////////////////////////////////
	// optimize coordinate vertexes
	/////////////////////////////////
	
	cs_CVertex *cv = new cs_CVertex[s->num_cvertex];
	for(int i = 0; i < s->num_cvertex; i++) {
		cv[i].vertex = s->vertex[i].xyz;
		cv[i].id = i;
	}
	qsort(cv,s->num_cvertex,sizeof(cs_CVertex),cs_cvertex_cmp);
	int num_optimized_cvertex = 0;
	int *cvbuf = new int[s->num_cvertex];
	for(int i = 0; i < s->num_cvertex; i++) {
		if(i == 0 || cs_cvertex_cmp(&cv[num_optimized_cvertex - 1],&cv[i])) cv[num_optimized_cvertex++] = cv[i];
		cvbuf[cv[i].id] = num_optimized_cvertex - 1;
	}
	
	/////////////////////////////////
	// create edges
	/////////////////////////////////
	
	cs_Edge *e = new cs_Edge[s->num_edges];
	for(int i = 0; i < s->num_edges; i += 3) {
		e[i + 0].v[0] = cvbuf[i + 1];
		e[i + 0].v[1] = cvbuf[i + 2];
		e[i + 1].v[0] = cvbuf[i + 2];
		e[i + 1].v[1] = cvbuf[i + 0];
		e[i + 2].v[0] = cvbuf[i + 0];
		e[i + 2].v[1] = cvbuf[i + 1];
		e[i + 0].face = i / 3;
		e[i + 1].face = i / 3;
		e[i + 2].face = i / 3;
		e[i + 0].num = 0;
		e[i + 1].num = 1;
		e[i + 2].num = 2;
		e[i + 0].id = i + 0;
		e[i + 1].id = i + 1;
		e[i + 2].id = i + 2;
	}
	qsort(e,s->num_edges,sizeof(cs_Edge),cs_edge_cmp);
	
	int num_optimized_edges = 0;
	int *ebuf = new int[s->num_edges];
	
	/////////////////////////////////
	// create and find environment triangles
	/////////////////////////////////
	
	s->triangles = new Triangle[s->num_triangles];
	for(int i = 0; i < s->num_triangles; i++) {
		s->triangles[i].t[0] = -1;
		s->triangles[i].t[1] = -1;
		s->triangles[i].t[2] = -1;
	}
	for(int i = 0, j = 0; i < s->num_edges; i++) {
		if(i == 0 || cs_edge_cmp(&e[i],&e[j])) j = i;
		if(i != j) {
			s->triangles[e[i].face].t[e[i].num] = e[j].face;
			s->triangles[e[j].face].t[e[j].num] = e[i].face;
		}
	}
	
	// optimize edges
	for(int i = 0; i < s->num_edges; i++) {
		if(i == 0 || cs_edge_cmp(&e[num_optimized_edges - 1],&e[i])) e[num_optimized_edges++] = e[i];
		ebuf[e[i].id] = num_optimized_edges - 1;
	}
	
	/////////////////////////////////
	// recreate vertexes and edges
	/////////////////////////////////
	
	// delete input vertexes
	delete [] s->vertex;
	
	// create optimized vertexes
	s->num_vertex = num_optimized_vertex;
	s->vertex = new Vertex[s->num_vertex];
	for(int i = 0; i < s->num_vertex; i++) {
		s->vertex[i] = v[i].vertex;
	}
	
	// create optimized coordinate vertexes
	s->num_cvertex = num_optimized_cvertex;
	s->cvertex = new vec3[s->num_cvertex];
	for(int i = 0; i < s->num_cvertex; i++) {
		s->cvertex[i] = cv[i].vertex;
	}
	
	// create optimized edges
	s->num_edges = num_optimized_edges;
	s->edges = new Edge[s->num_edges];
	for(int i = 0; i < s->num_edges; i++) {
		s->edges[i].cv[0] = e[i].v[0];
		s->edges[i].cv[1] = e[i].v[1];
	}
	
	/////////////////////////////////
	// create triangles
	/////////////////////////////////
	
	int num_optimized_triangles = 0;
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[num_optimized_triangles];
		t->v[0] = vbuf[j + 0];
		t->v[1] = vbuf[j + 1];
		t->v[2] = vbuf[j + 2];
		t->cv[0] = cvbuf[j + 0];
		t->cv[1] = cvbuf[j + 1];
		t->cv[2] = cvbuf[j + 2];
		t->e[0] = ebuf[j + 0];
		t->e[1] = ebuf[j + 1];
		t->e[2] = ebuf[j + 2];
		vec3 normal;	
		normal = normalize(cross(s->cvertex[t->cv[1]] - s->cvertex[t->cv[0]],s->cvertex[t->cv[2]] - s->cvertex[t->cv[0]]));
		if(length(normal) < EPSILON) continue;
		t->plane = vec4(normal,-s->cvertex[t->cv[0]] * normal);
		normal = normalize(cross(t->plane,s->cvertex[t->cv[1]] - s->cvertex[t->cv[0]]));
		if(length(normal) < EPSILON) continue;
		t->c[0] = vec4(normal,-s->cvertex[t->cv[0]] * normal);
		normal = normalize(cross(t->plane,s->cvertex[t->cv[2]] - s->cvertex[t->cv[1]]));
		if(length(normal) < EPSILON) continue;
		t->c[1] = vec4(normal,-s->cvertex[t->cv[1]] * normal);
		normal = normalize(cross(t->plane,s->cvertex[t->cv[0]] - s->cvertex[t->cv[2]]));
		if(length(normal) < EPSILON) continue;
		t->c[2] = vec4(normal,-s->cvertex[t->cv[2]] * normal);
		num_optimized_triangles++;
	}
	s->num_triangles = num_optimized_triangles;
	
	// add surface
	surfaces.append(s);
	
	// calculate tangent space
	calculate_tangent(s);
	
	// calculate bounds
	calculate_bounds(s);
	
	delete [] v;
	delete [] vbuf;
	delete [] cv;
	delete [] cvbuf;
	delete [] e;
	delete [] ebuf;
}

/*****************************************************************************/
/*                                                                           */
/* mesh file format                                                          */
/*                                                                           */
/*****************************************************************************/

/* load mesh
 */
int Mesh::load_mesh(const char *name) {
	
	struct mesh_Vertex {
		vec3 xyz;
		vec3 normal;
		vec4 texcoord;
	};
	
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Mesh::load_mesh(): can't open \"%s\" file\n",name);
		return 0;
	}
	
	int magic;
	fread(&magic,sizeof(int),1,file);
	
	if(magic != MESH_INDICES_MAGIC) {
		fprintf(stderr,"Mesh::load_mesh(): wrong magic 0x%04x in \"%s\" file\n",magic,name);
		fclose(file);
		return 0;
	}
	
	int num_surfaces;
	fread(&num_surfaces,sizeof(int),1,file);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		
		// name
		fread(s->name,sizeof(s->name),1,file);
		
		// vertexes
		int num_vertex;
		fread(&num_vertex,sizeof(int),1,file);
		mesh_Vertex *vertex = new mesh_Vertex[num_vertex];
		fread(vertex,sizeof(mesh_Vertex),num_vertex,file);
		
		// triangles
		int num_triangles;
		fread(&num_triangles,sizeof(int),1,file);
		int *indices = new int[num_triangles * 3];
		fread(indices,sizeof(int),num_triangles * 3,file);
		
		// check surface
		if(num_vertex == 0 || num_triangles == 0) {
			delete [] indices;
			delete [] vertex;
			delete s;
			continue;
		}
		
		// create raw triangles
		s->num_vertex = num_triangles * 3;
		s->vertex = new Vertex[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			s->vertex[j].xyz = vertex[indices[j]].xyz;
			s->vertex[j].normal = vertex[indices[j]].normal;
			s->vertex[j].texcoord = vertex[indices[j]].texcoord;
		}
		
		delete [] vertex;
		delete [] indices;
		
		create_surface(s);
	}
	
	fclose(file);
	return 1;
}

/* save mesh
 */
int Mesh::save_mesh(const char *name) {
	
	struct mesh_Vertex {
		vec3 xyz;
		vec3 normal;
		vec4 texcoord;
	};
	
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"Mesh::save_mesh(): error create \"%s\" file\n",name);
		return 0;
	}
	
	// write magic
	int magic = MESH_INDICES_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	
	// write num_surfaces
	int num_surfaces = surfaces.size();
	fwrite(&num_surfaces,sizeof(int),1,file);
	
	for(int i = 0; i < surfaces.size(); i++) {
		Surface *s = surfaces[i];
		
		// name
		fwrite(s->name,sizeof(s->name),1,file);
		
		// vertexes
		fwrite(&s->num_vertex,sizeof(int),1,file);
		mesh_Vertex *vertex = new mesh_Vertex[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			vertex[j].xyz = s->vertex[j].xyz;
			vertex[j].normal = s->vertex[j].normal;
			vertex[j].texcoord = s->vertex[j].texcoord;
		}
		fwrite(vertex,sizeof(mesh_Vertex),s->num_vertex,file);
		
		// indices
		fwrite(&s->num_triangles,sizeof(int),1,file);
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			fwrite(&t->v[0],sizeof(int),1,file);
			fwrite(&t->v[1],sizeof(int),1,file);
			fwrite(&t->v[2],sizeof(int),1,file);
		}
		
		delete [] vertex;
	}
	
	fclose(file);
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* 3ds loader                                                                */
/*                                                                           */
/*****************************************************************************/

enum {
	LOAD_3DS_CHUNK_MAIN = 0x4d4d,
	LOAD_3DS_CHUNK_OBJMESH = 0x3d3d,
	LOAD_3DS_CHUNK_OBJBLOCK = 0x4000,
	LOAD_3DS_CHUNK_TRIMESH = 0x4100,
	LOAD_3DS_CHUNK_VERTLIST = 0x4110,
	LOAD_3DS_CHUNK_FACELIST = 0x4120,
	LOAD_3DS_CHUNK_MAPLIST = 0x4140,
	LOAD_3DS_CHUNK_SMOOTHLIST = 0x4150
};

struct load_3ds_trimesh {
	vec3 *vertex;
	int num_vertex;
	vec2 *texcoord;
	int num_texcoord;
	int *face;
	int *smoothgroup;
	int num_face;
	vec3 *normal;
};

struct load_3ds_objblock {
	char name[1024];
	Vector<load_3ds_trimesh*> trimeshes;
};

struct load_3ds_mesh {
	Vector<load_3ds_objblock*> objblocks;
};

typedef int (*load_3ds_process_chunk)(FILE *file,int type,int size,void *data);

static int load_3ds_skeep_bytes(FILE *file,int bytes) {
	fseek(file,bytes,SEEK_CUR);
	return bytes;
}

static int load_3ds_read_string(FILE *file,char *string) {
	int i = 0;
	char *s = string;
	while((*s++ = fgetc(file)) != '\0') i++;
	return ++i;
}

static int load_3ds_read_ushort(FILE *file) {
	unsigned short ret;
	fread(&ret,1,sizeof(unsigned short),file);
	return ret;
}

static int load_3ds_read_int(FILE *file) {
	int ret;
	fread(&ret,1,sizeof(int),file);
	return ret;
}

static float load_3ds_read_float(FILE *file) {
	float ret;
	fread(&ret,1,sizeof(float),file);
	return ret;
}

static int load_3ds_read_chunk(FILE *file,load_3ds_process_chunk func,void *data) {
	int type = load_3ds_read_ushort(file);
	int size = load_3ds_read_int(file);
	if(func(file,type,size - 6,data) == 0) load_3ds_skeep_bytes(file,size - 6);
	return size;
}

static int load_3ds_read_chunks(FILE *file,int bytes,load_3ds_process_chunk func,void *data) {
	int bytes_read = 0;
	while(bytes_read < bytes) bytes_read += load_3ds_read_chunk(file,func,data);
	if(bytes_read != bytes) fprintf(stderr,"Mesh::load_3ds(): expected %d bytes but read %d\n",bytes,bytes_read);
	return bytes_read;
}

static int load_3ds_process_smoothlist(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_SMOOTHLIST) {
		load_3ds_trimesh *t = (load_3ds_trimesh*)data;
		t->smoothgroup = new int[t->num_face];
		for(int i = 0; i < t->num_face; i++) t->smoothgroup[i] = load_3ds_read_int(file);
		return 1;
	}
	return 0;
}

static int load_3ds_process_trimesh(FILE *file,int type,int size,void *data) {
	load_3ds_trimesh *t = (load_3ds_trimesh*)data;
	if(type == LOAD_3DS_CHUNK_VERTLIST) {
		t->num_vertex = load_3ds_read_ushort(file);
		t->vertex = new vec3[t->num_vertex];
		for(int i = 0; i < t->num_vertex; i++) {
			t->vertex[i].x = load_3ds_read_float(file);
			t->vertex[i].y = load_3ds_read_float(file);
			t->vertex[i].z = load_3ds_read_float(file);
		}
		return 1;
	} else if(type == LOAD_3DS_CHUNK_MAPLIST) {
		t->num_texcoord = load_3ds_read_ushort(file);
		t->texcoord = new vec2[t->num_texcoord];
		for(int i = 0; i < t->num_texcoord; i++) {
			t->texcoord[i].x = load_3ds_read_float(file);
			t->texcoord[i].y = 1.0f - load_3ds_read_float(file);
		}
		return 1;
	} else if(type == LOAD_3DS_CHUNK_FACELIST) {
		int bytes_left;
		t->num_face = load_3ds_read_ushort(file);
		t->face = new int[t->num_face * 3];
		for(int i = 0; i < t->num_face * 3; i += 3) {
			t->face[i + 0] = load_3ds_read_ushort(file);
			t->face[i + 1] = load_3ds_read_ushort(file);
			t->face[i + 2] = load_3ds_read_ushort(file);
			load_3ds_read_ushort(file);
		}
		bytes_left = size - t->num_face * sizeof(unsigned short) * 4 - 2;
		if(bytes_left > 0) load_3ds_read_chunks(file,bytes_left,load_3ds_process_smoothlist,t);
		return 1;
	}
	return 0;
}

static int load_3ds_process_objblock(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_TRIMESH) {
		load_3ds_objblock *o = (load_3ds_objblock*)data;
		load_3ds_trimesh *t = new load_3ds_trimesh;
		o->trimeshes.append(t);
		memset(t,0,sizeof(load_3ds_trimesh));
		load_3ds_read_chunks(file,size,load_3ds_process_trimesh,t);
		return 1;
	}
	return 0;
}

static int load_3ds_process_objmesh(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_OBJBLOCK) {
		load_3ds_mesh *m = (load_3ds_mesh*)data;
		load_3ds_objblock *o = new load_3ds_objblock;
		m->objblocks.append(o);
		memset(o,0,sizeof(load_3ds_objblock));
		size -= load_3ds_read_string(file,o->name);
		load_3ds_read_chunks(file,size,load_3ds_process_objblock,o);
		return 1;
	}
	return 0;
}

static int load_3ds_process_main(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_OBJMESH) {
		load_3ds_read_chunks(file,size,load_3ds_process_objmesh,data);
		return 1;
	}
	return 0;
}

static void load_3ds_trimesh_calculate_normals(load_3ds_trimesh *t) {
	vec3 *normal_face = new vec3[t->num_face];
	int *vertex_count = new int[t->num_vertex];
	memset(vertex_count,0,sizeof(int) * t->num_vertex);
	int **vertex_face = new int*[t->num_vertex];
	t->normal = new vec3[t->num_face * 3];
	memset(t->normal,0,sizeof(vec3) * t->num_face * 3);
	if(t->texcoord == NULL) {
		t->num_texcoord = t->num_vertex;
		t->texcoord = new vec2[t->num_vertex];
		memset(t->texcoord,0,sizeof(vec2) * t->num_vertex);
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		vertex_count[v0]++;
		vertex_count[v1]++;
		vertex_count[v2]++;
		normal_face[i].cross(t->vertex[v1] - t->vertex[v0],t->vertex[v2] - t->vertex[v0]);
		normal_face[i].normalize();
	}
	for(int i = 0; i < t->num_vertex; i++) {
		vertex_face[i] = new int[vertex_count[i] + 1];
		vertex_face[i][0] = vertex_count[i];
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		vertex_face[v0][vertex_count[v0]--] = i;
		vertex_face[v1][vertex_count[v1]--] = i;
		vertex_face[v2][vertex_count[v2]--] = i;
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		for(int k = 1; k <= vertex_face[v0][0]; k++) {
			int l = vertex_face[v0][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 0] += normal_face[l];
		}
		for(int k = 1; k <= vertex_face[v1][0]; k++) {
			int l = vertex_face[v1][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 1] += normal_face[l];
		}
		for(int k = 1; k <= vertex_face[v2][0]; k++) {
			int l = vertex_face[v2][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 2] += normal_face[l];
		}
	}
	for(int i = 0; i < t->num_face * 3; i++) t->normal[i].normalize();
	for(int i = 0; i < t->num_vertex; i++) delete vertex_face[i];
	delete [] vertex_face;
	delete [] vertex_count;
	delete [] normal_face;
}

/* load 3ds
 */
int Mesh::load_3ds(const char *name) {
	
	FILE *file;
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Mesh::load_3ds(): can't open \"%s\" file\n",name);
		return 0;
	}
	
	int type = load_3ds_read_ushort(file);
	int size = load_3ds_read_int(file);
	if(type != LOAD_3DS_CHUNK_MAIN) {
		fprintf(stderr,"Mesh::load_3ds(): wrong main chunk in \"%s\" file\n",name);
		fclose(file);
		return 0;
	}
	
	load_3ds_mesh *m = new load_3ds_mesh;
	memset(m,0,sizeof(load_3ds_mesh));
	
	load_3ds_read_chunks(file,size - 6,load_3ds_process_main,m);
	
	for(int i = 0; i < m->objblocks.size(); i++) {
		load_3ds_objblock *o = m->objblocks[i];
		
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		
		// name
		strcpy(s->name,o->name);
		
		// calculate number of vertexes
		int num_objblock_vertex = 0;
		for(int j = 0; j < o->trimeshes.size(); j++) {
			num_objblock_vertex += o->trimeshes[j]->num_face * 3;
			load_3ds_trimesh_calculate_normals(o->trimeshes[j]);
		}
		s->num_vertex = num_objblock_vertex;
		s->vertex = new Vertex[s->num_vertex];
		
		// copy vertexes
		num_objblock_vertex = 0;
		for(int j = 0; j < o->trimeshes.size(); j++) {
			load_3ds_trimesh *t = o->trimeshes[j];
			
			for(int k = 0, l = 0; k < t->num_face; k++, l += 3) {
				Vertex *v = s->vertex + num_objblock_vertex + l;
				v[0].xyz = t->vertex[t->face[l + 0]];
				v[1].xyz = t->vertex[t->face[l + 1]];
				v[2].xyz = t->vertex[t->face[l + 2]];
				v[0].texcoord = vec4(t->texcoord[t->face[l + 0]].x,t->texcoord[t->face[l + 0]].y,0.0,0.0);
				v[1].texcoord = vec4(t->texcoord[t->face[l + 1]].x,t->texcoord[t->face[l + 1]].y,0.0,0.0);
				v[2].texcoord = vec4(t->texcoord[t->face[l + 2]].x,t->texcoord[t->face[l + 2]].y,0.0,0.0);
				v[0].normal = t->normal[l + 0];
				v[1].normal = t->normal[l + 1];
				v[2].normal = t->normal[l + 2];
			}
			
			num_objblock_vertex += t->num_face * 3;
			
			delete [] t->vertex;
			delete [] t->texcoord;
			delete [] t->face;
			if(t->smoothgroup) delete [] t->smoothgroup;
			delete [] t->normal;
			delete t;
		}
		
		o->trimeshes.clear();
		delete o;
		
		create_surface(s);
	}
	
	m->objblocks.clear();
	
	delete m;
	fclose(file);
	
	return 1;
}
