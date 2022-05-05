/* Shader
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

#ifndef __SHADER_H__
#define __SHADER_H__

#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

class Shader {
public:
	
	Shader(const char *name,const char *vertex = NULL,const char *fragment = NULL);
	~Shader();
	
	void bindNames(const char *name,...);
	
	void enable();
	void disable();
	void bind();
	void bind(const float *value,...);
	
	void setParam(const char *name,const float *value);
	void setParams(const float *value,...);
	
protected:
	
	struct Param {
		GLuint location;
		int length;
	};
	
	const char *error(char *data,int pos);
	const char *error();
	
	void getParam(const char *name,Param *param);
	
	GLhandleARB program;
	
	GLuint fragment_target;
	GLuint fragment_id;
	
	std::vector<Param> params;
};

#endif /* __SHADER_H__ */
