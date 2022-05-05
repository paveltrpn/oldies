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

#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "glext.h"
#include "particles.h"

vec3 Particles::off = vec3(0,0,1000000.0);

/*
 */
Particles::Particles(int num_particles,float radius,float speed,float rotation,float time) :
	num_particles(num_particles), radius(radius), speed(speed), rotation(rotation), time(time) {
	
	positions = new vec3[num_particles];
	speeds = new vec3[num_particles];
	colors = new vec3[num_particles];
	rotations = new float[num_particles];
	times = new float[num_particles];
	
	for(int i = 0; i < num_particles; i++) {
		positions[i] = off;
		speeds[i] = vec3(0,0,0);
		colors[i] = vec3(0,0,0);
		rotations[i] = 0;
		times[i] = (float)i / (float)num_particles * time;
	}
	
	num_vertex = num_particles * 4;
	vertex = new Vertex[num_vertex];
	
	for(int i = 0; i < num_vertex; i++) {
		vertex[i].xyz = vec4(positions[i / 4],0);
		vertex[i].texcoord = vec4(0,0,0,0);
		vertex[i].color = vec3(0,0,0);
	}
	
	min = off;
	max = off;
	center = off;
}

Particles::~Particles() {
	delete [] positions;
	delete [] speeds;
	delete [] colors;
	delete [] rotations;
	delete [] times;
	delete [] vertex;
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

void Particles::update(float ifps) {
	
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	
	for(int i = 0, j = 0; i < num_particles; i++, j += 4) {
		
		speeds[i] += force * ifps;
		positions[i] += speeds[i] * ifps;
		times[i] -= ifps;
		
		if(times[i] < 0.0) {
			positions[i] = pos;
			speeds[i] = vec3(rand(),rand(),rand()) * speed;
			colors[i] = color;
			rotations[i] = rand() * rotation;
			times[i] += time;
			speeds[i] += force * ifps * 2.0f;
			positions[i] += speeds[i] * ifps * 2.0f;
		}
		
		// update vertexes
		vertex[j + 0].xyz = vec4(positions[i],radius);
		vertex[j + 1].xyz = vertex[j + 0].xyz;
		vertex[j + 2].xyz = vertex[j + 0].xyz;
		vertex[j + 3].xyz = vertex[j + 0].xyz;
		
		float s = sin(rotations[i] * times[i] * PI * 2.0f);
		float c = cos(rotations[i] * times[i] * PI * 2.0f);
		vertex[j + 0].texcoord = vec4(-0.5,-0.5,s,c);
		vertex[j + 1].texcoord = vec4(0.5,-0.5,s,c);
		vertex[j + 2].texcoord = vec4(0.5,0.5,s,c);
		vertex[j + 3].texcoord = vec4(-0.5,0.5,s,c);
		
		vertex[j + 0].color = colors[i] * times[i] / time;
		vertex[j + 1].color = vertex[j + 0].color;
		vertex[j + 2].color = vertex[j + 0].color;
		vertex[j + 3].color = vertex[j + 0].color;
		
		// bound box
		if(positions[i].z < off.z - 10000.0) {
			if(min.x > positions[i].x) min.x = positions[i].x;
			if(max.x < positions[i].x) max.x = positions[i].x;
			if(min.y > positions[i].y) min.y = positions[i].y;
			if(max.y < positions[i].y) max.y = positions[i].y;
			if(min.z > positions[i].z) min.z = positions[i].z;
			if(max.z < positions[i].z) max.z = positions[i].z;
		}
	}
	
	if(min.z > max.z || min.z > off.z - 10000.0) {
		min = off;
		max = off;
		center = off;
	} else {
		min -= vec3(radius,radius,radius);
		max += vec3(radius,radius,radius);
		center = (min + max) / 2.0f;
	}
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

int Particles::render() {
	
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	
	glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(Vertex),vertex->xyz);
	glVertexAttribPointerARB(1,4,GL_FLOAT,0,sizeof(Vertex),vertex->texcoord);
	glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),vertex->color);
	
	glDrawArrays(GL_QUADS,0,num_vertex);
	
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
	
	return num_particles * 2;
}

/*
 */
void Particles::set(const vec3 &pos) {
	this->pos = pos;
}

void Particles::setForce(const vec3 &force) {
	this->force = force;
}

void Particles::setColor(const vec3 &color) {
	this->color = color;
}

/*****************************************************************************/
/*                                                                           */
/* normal law                                                                */
/*                                                                           */
/*****************************************************************************/

float Particles::rand() {
	return sqrt(-2.0f * log((float)::rand() / RAND_MAX)) * sin(2.0f * PI * (float)::rand() / RAND_MAX);
}
