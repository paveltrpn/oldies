/* Particles
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

#ifndef __PARTICLES_H__
#define __PARTICLES_H__

#include "mathlib.h"

class Particles {
public:
	
	Particles(int num_particles,float radius,float speed,float rotation,float time);
	~Particles();
	
	void update(float ifps);
	
	int render();
	
	void set(const vec3 &pos);
	void setForce(const vec3 &force);
	void setColor(const vec3 &color);
	
	inline const vec3 &getMin() { return min; }
	inline const vec3 &getMax() { return max; }
	inline const vec3 &getCenter() { return center; }
	inline float getRadius() { return (max - center).length(); }
	
	static vec3 off;
	
protected:
	
	float rand();
	
	int num_particles;	// number of particles
	
	vec3 pos;			// position
	vec3 force;			// force
	vec3 color;			// color
	float radius;		// radius
	float speed;		// start speed
	float rotation;		// rotation speed
	float time;			// lifetime
	
	vec3 *positions;	// positions
	vec3 *speeds;		// speeds
	vec3 *colors;		// colors
	float *rotations;	// rotations
	float *times;		// times
	
	struct Vertex {
		vec4 xyz;		// coordinate + radius
		vec4 texcoord;	// texcoord + sin(rotation) + cos(rotation)
		vec3 color;		// color
	};
	
	int num_vertex;
	Vertex *vertex;
	
	vec3 min;			// bound box
	vec3 max;
	vec3 center;
};

#endif /* __PARTICLES_H__ */
