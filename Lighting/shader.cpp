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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "glext.h"
#include "shader.h"

Shader::Shader(const char *name,const char *vertex,const char *fragment) : program(0), fragment_target(0), fragment_id(0) {
	
	char *data;
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Shader::Shader(): can`t open \"%s\" file\n",name);
		return;
	}
	
	fseek(file,0,SEEK_END);
	int size = ftell(file);
	data = new char[size + 1];
	data[size] = '\0';
	fseek(file,0,SEEK_SET);
	fread(data,1,size,file);
	fclose(file);
	
	// skip comments
	char *s = data;
	char *d = data;
	while(*s) {
		if(*s == '/' && *(s + 1) == '/') {
			while(*s && *s != '\n') s++;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		}
		else if(*s == '/' && *(s + 1) == '*') {
			while(*s && (*s != '*' || *(s + 1) != '/')) s++;
			s += 2;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		}
		else *d++ = *s++;
	}
	*d = '\0';
	
	// find shaders
	char *vertex_src = NULL;
	char *fragment_src = NULL;
	s = data;
	while(*s) {
		if(*s == '<') {
			char *name = s;
			while(*s) {
				if(strchr("> \t\n\r",*s)) break;
				s++;
			}
			if(*s == '>') {	// ok it`s shader
				*name++ = '\0';
				*s++ = '\0';
				while(*s && strchr(" \t\n\r",*s)) s++;
				if(vertex == NULL && !strcmp(name,"vertex")) vertex_src = s;
				if(vertex && !strcmp(name,vertex)) vertex_src = s;
				if(fragment == NULL && !strcmp(name,"fragment")) fragment_src = s;
				if(fragment && !strcmp(name,fragment)) fragment_src = s;
			}
		}
		s++;
	}
	
	if(vertex_src) {
		
		program = glCreateProgramObjectARB();
		
		GLint length = (GLint)strlen(vertex_src);
		GLhandleARB vertex = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		glShaderSourceARB(vertex,1,(const GLcharARB**)&vertex_src,&length);
		glCompileShaderARB(vertex);
		glAttachObjectARB(program,vertex);
		glDeleteObjectARB(vertex);
		
		glBindAttribLocationARB(program,0,"s_attribute_0");
		glBindAttribLocationARB(program,1,"s_attribute_1");
		glBindAttribLocationARB(program,2,"s_attribute_2");
		glBindAttribLocationARB(program,3,"s_attribute_3");
		glBindAttribLocationARB(program,4,"s_attribute_4");
		glBindAttribLocationARB(program,5,"s_attribute_5");
		
		glBindAttribLocationARB(program,0,"s_xyz");
		glBindAttribLocationARB(program,1,"s_normal");
		glBindAttribLocationARB(program,2,"s_tangent");
		glBindAttribLocationARB(program,3,"s_binormal");
		glBindAttribLocationARB(program,4,"s_texcoord");
	}
	
	if(fragment_src) {
		
		if(!program) program = glCreateProgramObjectARB();
		
		if(!strncmp(fragment_src,"!!ARBfp1.0",10)) {
			fragment_target = GL_FRAGMENT_PROGRAM_ARB;
			glGenProgramsARB(1,&fragment_id);
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,fragment_id);
			glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(fragment_src),fragment_src);
			int pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&pos);
			if(pos != -1) {
				fprintf(stderr,"Shader::Shader(): fragment program error in \"%s\" file\n\"%s\"\n",name,error(data,pos));
				return;
			}
		}
		else if(!strncmp(fragment_src,"!!FP1.0",7)) {
			fragment_target = GL_FRAGMENT_PROGRAM_NV;
			glGenProgramsNV(1,&fragment_id);
			glBindProgramNV(GL_FRAGMENT_PROGRAM_NV,fragment_id);
			glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV,fragment_id,(GLsizei)strlen(fragment_src),(GLubyte*)fragment_src);
			int pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV,&pos);
			if(pos != -1) {
				fprintf(stderr,"Shader::Shader(): fragment program error in \"%s\" file\n\"%s\"\n",name,error(data,pos));
				return;
			}
		}
		else {
			GLint length = (GLint)strlen(fragment_src);
			GLhandleARB fragment = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
			glShaderSourceARB(fragment,1,(const GLcharARB**)&fragment_src,&length);
			glCompileShaderARB(fragment);
			glAttachObjectARB(program,fragment);
			glDeleteObjectARB(fragment);
		}
	}
	
	if(program) {
		glLinkProgramARB(program);
		GLint linked;
		glGetObjectParameterivARB(program,GL_OBJECT_LINK_STATUS_ARB,&linked);
		if(!linked) {
			fprintf(stderr,"Shader::Shader(): GLSL error in \"%s\" file\n%s\n",name,error());
			return;
		}
		
		glValidateProgramARB(program);
		GLint validated;
		glGetObjectParameterivARB(program,GL_OBJECT_VALIDATE_STATUS_ARB,&validated);
		if(!validated) {
			fprintf(stderr,"Shader::Shader(): GLSL error in \"%s\" file\n%s\n",name,error());
			return;
		}
		
		glUseProgramObjectARB(program);
		
		for(int i = 0; i < 8; i++) {
			char texture[32];
			sprintf(texture,"s_texture_%d",i);
			GLint location = glGetUniformLocationARB(program,texture);
			if(location >= 0) glUniform1iARB(location,i);
		}
		
		glUseProgramObjectARB(0);
	}
	
	delete data;
}

Shader::~Shader() {
	glDeleteObjectARB(program);
	params.clear();
}

/*
 */
const char *Shader::error(char *data,int pos) {
	char *s = data;
	while(*s && pos--) s++;
	while(s >= data && *s != '\n') s--;
	char *e = ++s;
	while(*e != '\0' && *e != '\n') e++;
	*e = '\0';
	return s;
}

/*
 */
const char *Shader::error() {
	int length;
	static char error[4096];
	glGetInfoLogARB(program,sizeof(error),&length,error);
	return error;
}

/*
 */
void Shader::getParam(const char *name,Param *param) {
	char buf[1024];
	strcpy(buf,name);
	char *s = strchr(buf,':');
	if(s) {
		*s++ = '\0';
		param->length = atoi(s);
	} else {
		param->length = 4;
	}
	param->location = glGetUniformLocationARB(program,buf);
}

/*
 */
void Shader::bindNames(const char *name,...) {
	Param param;
	getParam(name,&param);
	params.push_back(param);
	va_list args;
	va_start(args,name);
	while(1) {
		const char *name = va_arg(args,const char*);
		if(name == NULL) break;
		getParam(name,&param);
		params.push_back(param);
	}
	va_end(args);
}

/*****************************************************************************/
/*                                                                           */
/* enable/disable/bind                                                       */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Shader::enable() {
	if(fragment_id) glEnable(fragment_target);
}

/*
 */
void Shader::disable() {
	if(program) glUseProgramObjectARB(0);
	if(fragment_id) glDisable(fragment_target);
}

/*
 */
void Shader::bind() {
	if(program) glUseProgramObjectARB(program);
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramARB(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(fragment_target,fragment_id);
	}
}

/*
 */
void Shader::bind(const float *value,...) {
	
	glUseProgramObjectARB(program);
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramARB(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(fragment_target,fragment_id);
	}
	
	if(params[0].length == 1) glUniform1fvARB(params[0].location,1,value);
	else if(params[0].length == 2) glUniform2fvARB(params[0].location,1,value);
	else if(params[0].length == 3) glUniform3fvARB(params[0].location,1,value);
	else if(params[0].length == 4) glUniform4fvARB(params[0].location,1,value);
	else if(params[0].length == 9) glUniformMatrix3fvARB(params[0].location,1,false,value);
	else if(params[0].length == 16) glUniformMatrix4fvARB(params[0].location,1,false,value);
	va_list args;
	va_start(args,value);
	for(int i = 1; i < (int)params.size(); i++) {
		const float *value = va_arg(args,const float*);
		if(!value) break;
		if(params[i].length == 1) glUniform1fvARB(params[i].location,1,value);
		else if(params[i].length == 2) glUniform2fvARB(params[i].location,1,value);
		else if(params[i].length == 3) glUniform3fvARB(params[i].location,1,value);
		else if(params[i].length == 4) glUniform4fvARB(params[i].location,1,value);
		else if(params[i].length == 9) glUniformMatrix3fvARB(params[i].location,1,false,value);
		else if(params[i].length == 16) glUniformMatrix4fvARB(params[i].location,1,false,value);
	}
	va_end(args);
}

/*****************************************************************************/
/*                                                                           */
/* set parameters                                                            */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Shader::setParam(const char *name,const float *value) {
	Param param;
	getParam(name,&param);
	if(param.length == 1) glUniform1fvARB(param.location,1,value);
	else if(param.length == 2) glUniform2fvARB(param.location,1,value);
	else if(param.length == 3) glUniform3fvARB(param.location,1,value);
	else if(param.length == 4) glUniform4fvARB(param.location,1,value);
	else if(param.length == 9) glUniformMatrix3fvARB(param.location,1,false,value);
	else if(param.length == 16) glUniformMatrix4fvARB(param.location,1,false,value);
}

/*
 */
void Shader::setParams(const float *value,...) {
	if(params[0].length == 1) glUniform1fvARB(params[0].location,1,value);
	else if(params[0].length == 2) glUniform2fvARB(params[0].location,1,value);
	else if(params[0].length == 3) glUniform3fvARB(params[0].location,1,value);
	else if(params[0].length == 4) glUniform4fvARB(params[0].location,1,value);
	else if(params[0].length == 9) glUniformMatrix3fvARB(params[0].location,1,false,value);
	else if(params[0].length == 16) glUniformMatrix4fvARB(params[0].location,1,false,value);
	va_list args;
	va_start(args,value);
	for(int i = 1; i < (int)params.size(); i++) {
		const float *value = va_arg(args,const float*);
		if(!value) break;
		if(params[i].length == 1) glUniform1fvARB(params[i].location,1,value);
		else if(params[i].length == 2) glUniform2fvARB(params[i].location,1,value);
		else if(params[i].length == 3) glUniform3fvARB(params[i].location,1,value);
		else if(params[i].length == 4) glUniform4fvARB(params[i].location,1,value);
		else if(params[i].length == 9) glUniformMatrix3fvARB(params[i].location,1,false,value);
		else if(params[i].length == 16) glUniformMatrix4fvARB(params[i].location,1,false,value);
	}
	va_end(args);
}
