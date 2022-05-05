/* Spline
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

#include <stdio.h>
#include "spline.h"

Spline::Spline(const char *name,int close,float tension,float bias,float continuity) : num_points(0), params(NULL), length(0.0) {
	
	FILE *file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"Spline::Spline(): error open \"%s\" file\n",name);
		return;
	}
	
	vec3 v;
	int num_points = 0;
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) num_points++;
	
	vec3 *points = new vec3[num_points];
	num_points = 0;
	
	fseek(file,0,SEEK_SET);
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) points[num_points++] = v;
	fclose(file);
	
	create(points,num_points,close,tension,bias,continuity);
	
	delete [] points;
}

Spline::Spline(const vec3 *points,int num_points,int close,float tension,float bias,float continuity) : num_points(0), params(NULL), length(0.0) {
	create(points,num_points,close,tension,bias,continuity);
}

Spline::~Spline() {
	if(params) delete [] params;
}

/*
 */
void Spline::create(const vec3 *points,int num_points,int close,float tension,float bias,float continuity) {
	this->num_points = num_points;
	params = new vec3[num_points * 4];
	length = 0;
	for(int i = 0; i < num_points; i++) {
		vec3 prev,cur,next;
		if(i == 0) {
			if(close) prev = points[num_points - 1];
			else prev = points[i];
			cur = points[i];
			next = points[i + 1];
		} else if(i == num_points - 1) {
			prev = points[i - 1];
			cur = points[i];
			if(close) next = points[0];
			else next = points[i];
		} else {
			prev = points[i - 1];
			cur = points[i];
			next = points[i + 1];
		}
		vec3 p0 = (cur - prev) * (1.0f + bias);
		vec3 p1 = (next - cur) * (1.0f - bias);
		vec3 r0 = (p0 + (p1 - p0) * 0.5f * (1.0f + continuity)) * (1.0f - tension);
		vec3 r1 = (p0 + (p1 - p0) * 0.5f * (1.0f - continuity)) * (1.0f - tension);
		params[i * 4 + 0] = cur;
		params[i * 4 + 1] = next;
		params[i * 4 + 2] = r0;
		if(i) params[i * 4 - 1] = r1;
		else params[(num_points - 1) * 4 + 3] = r1;
		length += (next - cur).length();
	}
	for(int i = 0; i < num_points; i++) {
		vec3 p0 = params[i * 4 + 0];
		vec3 p1 = params[i * 4 + 1];
		vec3 r0 = params[i * 4 + 2];
		vec3 r1 = params[i * 4 + 3];
		params[i * 4 + 0] = p0;
		params[i * 4 + 1] = r0;
		params[i * 4 + 2] = -p0 * 3.0f + p1 * 3.0f - r0 * 2.0f - r1;
		params[i * 4 + 3] = p0 * 2.0f - p1 * 2.0f + r0 + r1;
	}
}

/*
 */
vec3 Spline::operator()(float t) {
	if(!num_points) return vec3(0,0,0);
	t *= num_points;
	int i = (int)t;
	t -= i;
	i = (i % num_points) * 4;
	float t2 = t * t;
	float t3 = t2 * t;
	return params[i + 0] + params[i + 1] * t + params[i + 2] * t2 + params[i + 3] * t3;
}

/*
 */
vec3 Spline::operator()(float t,float speed) {
	return operator()(t * speed / length);
}
