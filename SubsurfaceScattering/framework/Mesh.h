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

#ifndef __MESH_H__
#define __MESH_H__

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "Vector.h"
#include "MathLib.h"

#define MESH_INDICES_MAGIC ('m' | 'i' << 8 | '0' << 16 | '4' << 24)

/*
 */
class Mesh {
	
	public:
		
		Mesh();
		Mesh(const char *name);
		virtual ~Mesh();
		
		// render
		void enable();
		void disable();
		void bind(int surface);
		int render(int surface);
		int render();
		
		// render vertex clip
		void enableVertexClip();
		void disableVertexClip();
		void bindVertexClip(int surface);
		int renderVertexClip(int surface);
		int renderVertexClip();
		
		// shadow volumes
		int getNumIntersections(const vec3 &light,const vec3 &camera,float delta,int surface = -1);
		
		// render shadow volumes
		void enableShadowVolume();
		void disableShadowVolume();
		void bindShadowVolume(int surface);
		void bindShadowVolumeCaps(int surface);
		int renderShadowVolume(int surface);
		int renderShadowVolumeCaps(int surface);
		int renderShadowVolume();
		int renderShadowVolumeCaps();
		
		// line intersection
		int intersection(const vec3 &p0,const vec3 &p1,vec3 &ret_point,vec3 &ret_normal,int surface = -1);
		int intersection(const vec3 &p0,const vec3 &p1,vec3 &ret_point,vec3 &ret_normal,int &ret_surface,int &ret_triangle,int surface = -1);
		
		// io
		struct Vertex;
		struct Edge;
		struct Triangle;
		
		int load(const char *name);
		int save(const char *name);
		
		// add surface
		void addSurface(Mesh *mesh,int surface);
		void addSurface(const char *name,Vertex *vertex,int num_vertex);
	
		// get surface
		Vertex *getSurface(int surface,int &num_vertex);
		
		// find surface by name
		int findSurface(const char *name);
		
		// transformation
		void setTransform(const mat4 &m,int surface = -1);
		
		inline int getNumSurfaces() { return surfaces.size(); }
		inline const char *getSurfaceName(int surface) { return surfaces[surface]->name; }
		
		inline int getNumVertex(int surface) { return surfaces[surface]->num_vertex; }
		inline Vertex *getVertex(int surface) { return surfaces[surface]->vertex; }
		
		inline int getNumCVertex(int surface) { return surfaces[surface]->num_cvertex; }
		inline vec3 *getCVertex(int surface) { return surfaces[surface]->cvertex; }
		
		inline int getNumEdges(int surface) { return surfaces[surface]->num_edges; }
		inline Edge *getEdges(int surface) { return surfaces[surface]->edges; }
		
		inline int getNumTriangles(int surface) { return surfaces[surface]->num_triangles; }
		inline Triangle *getTriangles(int surface) { return surfaces[surface]->triangles; }
		
		inline const vec3 &getMin(int surface = -1) {
			if(surface == -1) return min;
			return surfaces[surface]->min;
		}
		inline const vec3 &getMax(int surface = -1) {
			if(surface == -1) return max;
			return surfaces[surface]->max;
		}
		
		inline const vec3 &getCenter(int surface = -1) {
			if(surface == -1) return center;
			return surfaces[surface]->center;
		}
		inline float getRadius(int surface = -1) {
			if(surface == -1) return radius;
			return surfaces[surface]->radius;
		}
		
		struct Vertex {
			vec3 xyz;		// cooridnate
			vec3 normal;	// normal
			vec3 tangent;	// tangent
			vec3 binormal;	// binormal
			vec4 texcoord;	// texture coordinates
		};
			
		struct VertexClip {
			Vertex vertex;	// ordinary vertex
			vec4 plane;		// triangle plane
			vec4 center;	// center of the triangle and radius
		};
		
		struct VertexShadowVolume {
			vec4 xyz;		// coordinate
			vec4 plane;		// triangle plane
		};
		
		struct Edge {
			int cv[2];		// coordinate vertexes
		};
		
		struct Triangle {
			int v[3];		// vertexes
			int cv[3];		// coordinate vertexes
			int t[3];		// nearest triangles
			int e[3];		// edges
			vec4 plane;		// plane
			vec4 c[3];		// fast point in triangle
			float dot;		// dot product
		};
		
	protected:
		
		struct Surface;
		
		void calculate_tangent(Surface *s);
		void calculate_bounds(Surface *s);
		void create_vbo(Surface *s);
		void create_vertex_clip_vbo(Surface *s);
		void create_shadow_volume_vbo(Surface *s);
		void create_surface(Surface *s);
		
		int load_mesh(const char *name);
		int save_mesh(const char *name);
		
		int load_3ds(const char *name);
		
		struct Surface {
			char name[128];				// surface name
			
			int num_vertex;				// vertexes
			Vertex *vertex;
			
			int num_cvertex;			// coordinate vertexes
			vec3 *cvertex;
			
			int num_edges;				// edges
			Edge *edges;
			
			int num_triangles;			// triangles
			Triangle *triangles;
			
			vec3 min;					// bound box
			vec3 max;
			vec3 center;				// bound sphere
			float radius;
				
			GLuint vertex_vbo_id;		// vbos
			GLuint indices_vbo_id;
			GLuint vertex_clip_vbo_id;
			
			GLuint shadow_volume_vertex_vbo_id;
			int shadow_volume_num_indices;
			GLuint shadow_volume_indices_vbo_id;
			int shadow_volume_num_caps_indices;
			GLuint shadow_volume_caps_indices_vbo_id;
		};
		
		vec3 min;
		vec3 max;
		vec3 center;
		float radius;
		
		Vector<Surface*> surfaces;
};

#endif /* __MESH_H__ */
