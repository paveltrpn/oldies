/* Mesh
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
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

#include "glext.h"
#include "mesh.h"

Mesh::Mesh() {
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	center = vec3(0,0,0);
	radius = 1000000;
}

Mesh::Mesh(const char *name) {
	load(name);
}

/*
 */
Mesh::~Mesh() {
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s = surfaces[i];
		delete [] s->vertex;
		delete [] s->cvertex;
		delete [] s->edges;
		delete [] s->triangles;
		for(int j = 0; j < NUM_SILHOUETTES; j++) {
			delete [] s->silhouettes[j].indices;
			delete [] s->silhouettes[j].flags;
		}
		delete [] s->silhouettes;
		if(s->vertex_id) glDeleteBuffersARB(1,&s->vertex_id);
		if(s->indices_id) glDeleteBuffersARB(1,&s->indices_id);
		if(s->clip_vertex_id) glDeleteBuffersARB(1,&s->clip_vertex_id);
		if(s->shadow_volume_vertex_id) glDeleteBuffersARB(1,&s->shadow_volume_vertex_id);
		delete s;
	}
	surfaces.clear();
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::render(int surface) {
	
	int num_triangles = 0;
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glEnableVertexAttribArrayARB(4);
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_id);
		glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),0);
		glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
		glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 2));
		glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 3));
		glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 4));

		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->indices_id);
		
		glDrawElements(GL_TRIANGLES,s->num_triangles * 3,GL_UNSIGNED_INT,0);
		
		num_triangles += s->num_triangles;
	}
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(4);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
	
	return num_triangles;
}

/*
 */
int Mesh::render(const vec3 &light,float radius,int surface) {
	
	int num_triangles = 0;
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glEnableVertexAttribArrayARB(4);
	glEnableVertexAttribArrayARB(5);
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		if((s->center - light).length() > s->radius + radius) continue;
		
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->clip_vertex_id);
		glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(clip_Vertex),0);
		glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(clip_Vertex),(void*)(sizeof(vec3) * 1));
		glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(clip_Vertex),(void*)(sizeof(vec3) * 2));
		glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(clip_Vertex),(void*)(sizeof(vec3) * 3));
		glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(clip_Vertex),(void*)(sizeof(vec3) * 4));
		glVertexAttribPointerARB(5,4,GL_FLOAT,0,sizeof(clip_Vertex),(void*)(sizeof(vec3) * 4 + sizeof(vec2)));
		
		glDrawArrays(GL_TRIANGLES,0,s->num_triangles * 3);
		
		num_triangles += s->num_triangles;
	}
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(5);
	glDisableVertexAttribArrayARB(4);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
	
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* shadow volumes                                                            */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::findSilhouette(const vec3 &light,float radius,int surface) {
	
	int ret = 0;
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		s->silhouette = NULL;
		if((s->center - light).length() > s->radius + radius) continue;
		ret = 1;
		
		// attempt to find the prepared silhouette
		for(int j = 0; j < NUM_SILHOUETTES; j++) {
			if(s->silhouettes[j].light == light && s->silhouettes[j].radius == radius) {
				s->silhouette = &s->silhouettes[j];
				break;
			}
		}
		if(s->silhouette) continue;
		
		s->silhouette = &s->silhouettes[s->last_silhouette++];
		if(s->last_silhouette == NUM_SILHOUETTES) s->last_silhouette = 0;

		s->silhouette->light = light;
		s->silhouette->radius = radius;
		s->silhouette->num_triangles = 0;
		int *indices = s->silhouette->indices;
		
		for(int j = 0; j < s->num_edges; j++) s->edges[j].flag = -1;
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			if(t->plane * light > 0.0) {
				s->silhouette->flags[j] = 1;
				s->edges[t->e[0]].reverse = t->reverse & (1 << 0);
				s->edges[t->e[1]].reverse = t->reverse & (1 << 1);
				s->edges[t->e[2]].reverse = t->reverse & (1 << 2);
				s->edges[t->e[0]].flag++;
				s->edges[t->e[1]].flag++;
				s->edges[t->e[2]].flag++;
			} else {
				s->silhouette->flags[j] = 0;
				*indices++ = t->cv[0] + s->num_cvertex;
				*indices++ = t->cv[1] + s->num_cvertex;
				*indices++ = t->cv[2] + s->num_cvertex;
				s->silhouette->num_triangles++;
			}
		}
		for(int j = 0; j < s->num_edges; j++) {
			Edge *e = &s->edges[j];
			if(e->flag != 0) continue;
			if(e->reverse) {
				*indices++ = e->cv[0];
				*indices++ = e->cv[1];
				*indices++ = e->cv[1] + s->num_cvertex;
				*indices++ = e->cv[1] + s->num_cvertex;
				*indices++ = e->cv[0] + s->num_cvertex;
				*indices++ = e->cv[0];
			} else {
				*indices++ = e->cv[0];
				*indices++ = e->cv[1] + s->num_cvertex;
				*indices++ = e->cv[1];
				*indices++ = e->cv[1] + s->num_cvertex;
				*indices++ = e->cv[0];
				*indices++ = e->cv[0] + s->num_cvertex;
			}
			s->silhouette->num_triangles += 2;
		}
	}
	
	return ret;
}

/*
 */
int Mesh::getNumIntersections(const vec3 &light,float radius,const vec3 &camera,int surface) {
	
	int num_intersections = 0;
	vec3 dir = camera - light;
	
	if(dir.length() > radius) return num_intersections;
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		if(s->silhouette == NULL) continue;
		if((s->center - light).length() > s->radius + radius) continue;
		
		float dot = (dir * s->center - dir * light) / (dir * dir);
		if(dot < 0.0 && (light - s->center).length() > s->radius) continue;
		else if(dot > 1.0 && (camera - s->center).length() > s->radius) continue;
		else if((light + dir * dot - s->center).length() > s->radius) continue;
		
		for(int j = 0; j < s->num_triangles; j++) {
			if(s->silhouette->flags[j] == 0) continue;
			Triangle *t = &s->triangles[j];
			float dot = -(t->plane * light) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = light + dir * dot;
			if(p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) num_intersections++;
		}
	}
	
	return num_intersections;
}

/*
 */
int Mesh::renderShadowVolume(int surface) {
	
	int num_triangles = 0;
	glEnableVertexAttribArrayARB(0);
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		if(!s->silhouette) continue;
		
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->shadow_volume_vertex_id);
		glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(vec4),0);
		
		glDrawElements(GL_TRIANGLES,s->silhouette->num_triangles * 3,GL_UNSIGNED_INT,s->silhouette->indices);
		
		num_triangles += s->num_triangles;
	}
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	glDisableVertexAttribArrayARB(0);
	
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
	int ret = intersection(p0,p1,ret_surface,ret_triangle,ret_point,surface);
	if(ret) ret_normal = surfaces[ret_surface]->triangles[ret_triangle].plane;
	return ret;
}

/*
 */
int Mesh::intersection(const vec3 &p0,const vec3 &p1,int &ret_surface,int &ret_triangle,vec3 &ret_point,int surface) {
	
	float nearest = 2.0;
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s;
		if(surface == -1) s = surfaces[i];
		else {
			if(i != 0) break;
			s = surfaces[surface];
		}
		
		vec3 dir = p1 - p0;
		
		float dot = (dir * s->center - dir * p0) / (dir * dir);
		if(dot < 0.0 && (p0 - s->center).length() > s->radius) continue;
		else if(dot > 1.0 && (p1 - s->center).length() > s->radius) continue;
		else if((p0 + dir * dot - s->center).length() > s->radius) continue;
		
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			float dot = -(t->plane * p0) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = p0 + dir * dot;
			if(nearest > dot && p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) {
				nearest = dot;
				ret_surface = i;
				ret_triangle = j;
				ret_point = p;
			}
		}
	}
	
	if(surface != -1) ret_surface = surface;
	
	return nearest < 2.0 ? 1 : 0;
}

/*****************************************************************************/
/*                                                                           */
/* io                                                                        */
/*                                                                           */
/*****************************************************************************/

/* load
 */
int Mesh::load(const char *name) {
	if(strstr(name,".mesh")) return load_mesh(name);
	if(strstr(name,".3ds")) return load_3ds(name);
	fprintf(stderr,"Mesh::load(): unknown format of \"%s\" file\n",name);
	return 0;
}

/* save
 */
int Mesh::save(const char *name) {
	if(strstr(name,".mesh")) return save_mesh(name);
	fprintf(stderr,"Mesh::save(): unknown format of \"%s\" file\n",name);
	return 0;
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
	for(int i = 0; i < (int)surfaces.size(); i++) {
		if(!strcmp(name,surfaces[i]->name)) return i;
	}
	return -1;
}

/* transform mesh
 */
void Mesh::transform(const mat4 &m,int surface) {
	
	mat4 r = m.rotation();
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
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
		create_vbo(s);

		// recalculate bounds
		calculate_bounds(s);
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
	s->min = vec3(1000000,1000000,1000000);
	s->max = vec3(-1000000,-1000000,-1000000);
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
		float r = (s->center - s->cvertex[i]).length();
		if(r > s->radius) s->radius = r;
	}
	
	// recalculate bounds for the mesh
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Surface *s = surfaces[i];
		if(max.x < s->max.x) max.x = s->max.x;
		if(min.x > s->min.x) min.x = s->min.x;
		if(max.y < s->max.y) max.y = s->max.y;
		if(min.y > s->min.y) min.y = s->min.y;
		if(max.z < s->max.z) max.z = s->max.z;
		if(min.z > s->min.z) min.z = s->min.z;
	}
	center = (min + max) / 2.0f;
	radius = (max - center).length();
}

/*****************************************************************************/
/*                                                                           */
/* calculate tangent                                                         */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::calculate_tangent(Surface *s) {
	
	for(int i = 0; i < s->num_vertex; i += 3) {
		
		Vertex *v0 = &s->vertex[i + 0];
		Vertex *v1 = &s->vertex[i + 1];
		Vertex *v2 = &s->vertex[i + 2];
		
		vec3 tangent,binormal;
		vec3 e0 = vec3(0,v1->texcoord.x - v0->texcoord.x,v1->texcoord.y - v0->texcoord.y);
		vec3 e1 = vec3(0,v2->texcoord.x - v0->texcoord.x,v2->texcoord.y - v0->texcoord.y);
		for(int k = 0; k < 3; k++) {
			e0.x = v1->xyz[k] - v0->xyz[k];
			e1.x = v2->xyz[k] - v0->xyz[k];
			vec3 v;
			v.cross(e0,e1);
			if(fabs(v[0]) > EPSILON) {
				tangent[k] = -v[1] / v[0];
				binormal[k] = -v[2] / v[0];
			} else {
				tangent[k] = 0;
				binormal[k] = 0;
			}
		}
		
		tangent.normalize();
		binormal.normalize();
		vec3 normal = normalize(cross(tangent,binormal));
		
		v0->binormal = normalize(cross(v0->normal,tangent));
		v0->tangent = cross(v0->binormal,v0->normal);
		if(normal * v0->normal < 0.0) v0->binormal = -v0->binormal;
		
		v1->binormal = normalize(cross(v1->normal,tangent));
		v1->tangent = cross(v1->binormal,v1->normal);
		if(normal * v1->normal < 0.0) v1->binormal = -v1->binormal;
		
		v2->binormal = normalize(cross(v2->normal,tangent));
		v2->tangent = cross(v2->binormal,v2->normal);
		if(normal * v2->normal < 0.0) v2->binormal = -v2->binormal;
	}
}

/*****************************************************************************/
/*                                                                           */
/* create vertex buffer objects                                              */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::create_vbo(Surface *s) {
	
	if(s->vertex_id) glDeleteBuffersARB(1,&s->vertex_id);
	if(s->indices_id) glDeleteBuffersARB(1,&s->indices_id);
	if(s->clip_vertex_id) glDeleteBuffersARB(1,&s->clip_vertex_id);
	if(s->shadow_volume_vertex_id) glDeleteBuffersARB(1,&s->shadow_volume_vertex_id);
	
	// vertex
	glGenBuffersARB(1,&s->vertex_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->vertex_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(Vertex) * s->num_vertex,s->vertex,GL_STATIC_DRAW_ARB);
	
	// indices
	int *indices = new int[s->num_triangles * 3];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		indices[j + 0] = t->v[0];
		indices[j + 1] = t->v[1];
		indices[j + 2] = t->v[2];
	}
	glGenBuffersARB(1,&s->indices_id);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,s->indices_id);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,sizeof(int) * s->num_triangles * 3,indices,GL_STATIC_DRAW_ARB);
	delete [] indices;
	
	// clip vertexes
	clip_Vertex *clip_vertex = new clip_Vertex[s->num_triangles * 3];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		for(int k = 0; k < 3; k++) {
			clip_vertex[j + k].vertex.xyz = s->vertex[t->v[k]].xyz;
			clip_vertex[j + k].vertex.normal = s->vertex[t->v[k]].normal;
			clip_vertex[j + k].vertex.tangent = s->vertex[t->v[k]].tangent;
			clip_vertex[j + k].vertex.binormal = s->vertex[t->v[k]].binormal;
			clip_vertex[j + k].vertex.texcoord = s->vertex[t->v[k]].texcoord;
		}
		vec3 center = (s->vertex[t->v[0]].xyz + s->vertex[t->v[1]].xyz + s->vertex[t->v[2]].xyz) / 3.0f;
		float radius = 0;
		for(int k = 0; k < 3; k++) {
			float r = (s->vertex[t->v[k]].xyz - center).length();
			if(radius < r) radius = r;
		}
		clip_vertex[j + 0].center = vec4(center,radius);
		clip_vertex[j + 1].center = vec4(center,radius);
		clip_vertex[j + 2].center = vec4(center,radius);
	}
	glGenBuffersARB(1,&s->clip_vertex_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->clip_vertex_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(clip_Vertex) * s->num_triangles * 3,clip_vertex,GL_STATIC_DRAW_ARB);
	delete [] clip_vertex;
	
	// shadow volume vertexes
	vec4 *shadow_volume_vertex = new vec4[s->num_cvertex * 2];
	for(int i = 0; i < s->num_cvertex; i++) {
		shadow_volume_vertex[i] = vec4(s->cvertex[i],1);
		shadow_volume_vertex[i + s->num_cvertex] = vec4(s->cvertex[i],0);
	}
	glGenBuffersARB(1,&s->shadow_volume_vertex_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,s->shadow_volume_vertex_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(vec4) * s->num_cvertex * 2,shadow_volume_vertex,GL_STATIC_DRAW_ARB);
	delete [] shadow_volume_vertex;
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
}

/*****************************************************************************/
/*                                                                           */
/* create surface                                                            */
/*                                                                           */
/*****************************************************************************/

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
	int id;
};

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
	
	s->num_cvertex = s->num_vertex;
	s->num_edges = s->num_vertex;
	s->num_triangles = s->num_vertex / 3;
	
	// calculate tangent space
	calculate_tangent(s);
	
	// optimize vertexes
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
	
	// optimize coordinate vertexes
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
	
	// optimize edges
	cs_Edge *e = new cs_Edge[s->num_edges];
	for(int j = 0; j < s->num_edges; j += 3) {
		e[j + 0].v[0] = cvbuf[j + 0];
		e[j + 0].v[1] = cvbuf[j + 1];
		e[j + 1].v[0] = cvbuf[j + 1];
		e[j + 1].v[1] = cvbuf[j + 2];
		e[j + 2].v[0] = cvbuf[j + 2];
		e[j + 2].v[1] = cvbuf[j + 0];
		e[j + 0].id = j + 0;
		e[j + 1].id = j + 1;
		e[j + 2].id = j + 2;
	}
	qsort(e,s->num_edges,sizeof(cs_Edge),cs_edge_cmp);
	int num_optimized_edges = 0;
	int *ebuf = new int[s->num_edges];
	int *rbuf = new int[s->num_edges];
	for(int j = 0; j < s->num_edges; j++) {
		if(j == 0 || cs_edge_cmp(&e[num_optimized_edges - 1],&e[j])) e[num_optimized_edges++] = e[j];
		ebuf[e[j].id] = num_optimized_edges - 1;
		rbuf[e[j].id] = e[num_optimized_edges - 1].v[0] != e[j].v[0];
	}
	
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
		s->edges[i].reverse = 0;
		s->edges[i].flag = 0;
	}
	
	// create triangles
	s->triangles = new Triangle[s->num_triangles];
	for(int i = 0, j = 0; i < s->num_triangles; i++, j += 3) {
		Triangle *t = &s->triangles[i];
		
		t->v[0] = vbuf[j + 0];
		t->v[1] = vbuf[j + 1];
		t->v[2] = vbuf[j + 2];
		
		t->cv[0] = cvbuf[j + 0];
		t->cv[1] = cvbuf[j + 1];
		t->cv[2] = cvbuf[j + 2];
		
		t->e[0] = ebuf[j + 0];
		t->e[1] = ebuf[j + 1];
		t->e[2] = ebuf[j + 2];
		
		t->reverse = 0;
		if(rbuf[j + 0]) t->reverse |= (1 << 0);
		if(rbuf[j + 1]) t->reverse |= (1 << 1);
		if(rbuf[j + 2]) t->reverse |= (1 << 2);
		
		t->t[0] = -1;
		t->t[1] = -1;
		t->t[2] = -1;
		
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
	
	// create silhouettes
	s->silhouettes = new Silhouette[NUM_SILHOUETTES];
	memset(s->silhouettes,0,sizeof(Silhouette) * NUM_SILHOUETTES);
	for(int i = 0; i < NUM_SILHOUETTES; i++) {
		s->silhouettes[i].indices = new int[s->num_triangles * 12];
		s->silhouettes[i].flags = new int[s->num_triangles];
	}
	
	// create buffer objects
	create_vbo(s);
	
	// add surface
	surfaces.push_back(s);
	
	// calculate bounds
	calculate_bounds(s);
	
	delete [] v;
	delete [] vbuf;
	delete [] cv;
	delete [] cvbuf;
	delete [] e;
	delete [] ebuf;
	delete [] rbuf;
}

/*****************************************************************************/
/*                                                                           */
/* mesh file format                                                          */
/*                                                                           */
/*****************************************************************************/

struct mesh_Vertex {
	vec3 xyz;
	vec3 normal;
	vec2 texcoord;
};

/* load mesh
 */
int Mesh::load_mesh(const char *name) {
	
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Mesh::load_mesh(): error open \"%s\" file\n",name);
		return 0;
	}
	
	int magic;
	fread(&magic,sizeof(int),1,file);
	
	if(magic == MESH_STRIP_MAGIC) {
		fprintf(stderr,"Mesh::load_mesh(): strip mesh format is not supported\n");
	}
	else if(magic == MESH_RAW_MAGIC) {
		// number of surfaces
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
			
			// copy vertexes
			s->num_vertex = num_vertex;
			s->vertex = new Vertex[s->num_vertex];
			for(int j = 0; j < s->num_vertex; j++) {
				s->vertex[j].xyz = vertex[j].xyz;
				s->vertex[j].normal = vertex[j].normal;
				s->vertex[j].texcoord = vertex[j].texcoord;
			}
			
			delete [] vertex;
			
			create_surface(s);
		}
	}
	else if(magic == MESH_INDICES_MAGIC) {
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
	}
	
	else {
		fprintf(stderr,"Mesh::load_mesh(): wrong magic 0x%04x in \"%s\" file\n",magic,name);
		fclose(file);
		return 0;
	}
	
	fclose(file);
	return 1;
}

/* save mesh
 */
int Mesh::save_mesh(const char *name) {
	
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"Mesh::load_mesh(): error create \"%s\" file\n",name);
		return 0;
	}
	
	// write magic
	int magic = MESH_INDICES_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	
	// write num_surfaces
	int num_surfaces = (int)surfaces.size();
	fwrite(&num_surfaces,sizeof(int),1,file);
	
	for(int i = 0; i < (int)surfaces.size(); i++) {
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
	std::vector<load_3ds_trimesh*> trimeshes;
};

struct load_3ds_mesh {
	std::vector<load_3ds_objblock*> objblocks;
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
		o->trimeshes.push_back(t);
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
		m->objblocks.push_back(o);
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
		fprintf(stderr,"Mesh::load_3ds(): error open \"%s\" file\n",name);
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
	
	for(int i = 0; i < (int)m->objblocks.size(); i++) {
		load_3ds_objblock *o = m->objblocks[i];
		
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		
		// name
		strcpy(s->name,o->name);
		
		// calculate number of vertexes
		int num_objblock_vertex = 0;
		for(int j = 0; j < (int)o->trimeshes.size(); j++) {
			num_objblock_vertex += o->trimeshes[j]->num_face * 3;
			load_3ds_trimesh_calculate_normals(o->trimeshes[j]);
		}
		s->num_vertex = num_objblock_vertex;
		s->vertex = new Vertex[s->num_vertex];
		
		// copy vertexes
		num_objblock_vertex = 0;
		for(int j = 0; j < (int)o->trimeshes.size(); j++) {
			load_3ds_trimesh *t = o->trimeshes[j];
			
			for(int k = 0, l = 0; k < t->num_face; k++, l += 3) {
				Vertex *v = s->vertex + num_objblock_vertex + l;
				v[0].xyz = t->vertex[t->face[l + 0]];
				v[1].xyz = t->vertex[t->face[l + 1]];
				v[2].xyz = t->vertex[t->face[l + 2]];
				v[0].texcoord = t->texcoord[t->face[l + 0]];
				v[1].texcoord = t->texcoord[t->face[l + 1]];
				v[2].texcoord = t->texcoord[t->face[l + 2]];
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
