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

#ifndef __SPLINE_H__
#define __SPLINE_H__

#include "mathlib.h"

class Spline {
public:
	
	Spline(const char *name,int close = 0,float tension = 0.0,float bias = 0.0,float continuity = 0.0);
	Spline(const vec3 *points,int num_points,int close = 0,float tension = 0.0,float bias = 0.0,float continuity = 0.0);
	~Spline();
	
	vec3 operator()(float t);
	vec3 operator()(float t,float speed);
	
protected:
	
	void create(const vec3 *points,int num_points,int close,float tension,float bias,float continuity);
	
	int num_points;
	vec3 *params;
	float length;
};

#endif /* __SPLINE_H__ */
