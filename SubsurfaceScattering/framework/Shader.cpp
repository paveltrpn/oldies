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
#include <stdlib.h>
#include <string.h>

#include "GLExt.h"
#include "Shader.h"

Shader::Shader(const char *name,const char *vertex,const char *fragment) {
	
	program = 0;
	vertex_target = 0;
	vertex_id = 0;
	fragment_target = 0;
	fragment_id = 0;
	
	char *data;
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Shader::Shader(): can't open \"%s\" file\n",name);
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
			if(*s == '>') {		// it`s shader
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
		
		// ARB vertex program
		if(!strncmp(vertex_src,"!!ARBvp1.0",10)) {
			vertex_target = GL_VERTEX_PROGRAM_ARB;
			glGenProgramsARB(1,&vertex_id);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB,vertex_id);
			glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(vertex_src),vertex_src);
			int pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&pos);
			if(pos != -1) {
				fprintf(stderr,"Shader::Shader(): vertex program error in \"%s\" file\n\"%s\"\n",name,get_error(vertex_src,pos));
				return;
			}
		}
		
		// ARB vertex shader
		else {
		
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
			glBindAttribLocationARB(program,6,"s_attribute_6");
			
			glBindAttribLocationARB(program,0,"s_xyz");
			glBindAttribLocationARB(program,1,"s_normal");
			glBindAttribLocationARB(program,2,"s_tangent");
			glBindAttribLocationARB(program,3,"s_binormal");
			glBindAttribLocationARB(program,4,"s_texcoord");
		}
	}
	
	if(fragment_src) {
		
		// ARB fragment program
		if(!strncmp(fragment_src,"!!ARBfp1.0",10)) {
			fragment_target = GL_FRAGMENT_PROGRAM_ARB;
			glGenProgramsARB(1,&fragment_id);
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,fragment_id);
			glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(fragment_src),fragment_src);
			int pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&pos);
			if(pos != -1) {
				fprintf(stderr,"Shader::Shader(): fragment program error in \"%s\" file\n\"%s\"\n",name,get_error(fragment_src,pos));
				return;
			}
		}
		
		// ARB fragment shader
		else {
			
			if(!program) program = glCreateProgramObjectARB();
			
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
			fprintf(stderr,"Shader::Shader(): GLSL error in \"%s\" file\n%s\n",name,get_glsl_error());
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
		
		glValidateProgramARB(program);
		GLint validated;
		glGetObjectParameterivARB(program,GL_OBJECT_VALIDATE_STATUS_ARB,&validated);
		if(!validated) {
			fprintf(stderr,"Shader::Shader(): GLSL error in \"%s\" file\n%s\n",name,get_glsl_error());
			return;
		}
	}
	
	delete [] data;
}

Shader::~Shader() {
	if(program) glDeleteObjectARB(program);
	if(vertex_target == GL_VERTEX_PROGRAM_ARB) glDeleteProgramsARB(1,&vertex_id);
	if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glDeleteProgramsARB(1,&fragment_id);
	else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glDeleteProgramsNV(1,&fragment_id);
	parameters.clear();
}

/*
 */
const char *Shader::get_error(char *data,int pos) {
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
const char *Shader::get_glsl_error() {
	int length;
	static char error[4096];
	glGetInfoLogARB(program,sizeof(error),&length,error);
	return error;
}

/*
 */
void Shader::getParameter(const char *name,Parameter *parameter) {
	if(program == 0) return;
	char buf[1024];
	strcpy(buf,name);
	char *s = strchr(buf,':');
	if(s) {
		*s++ = '\0';
		parameter->length = atoi(s);
	} else {
		parameter->length = 4;
	}
	parameter->location = glGetUniformLocationARB(program,buf);
}

/*
 */
void Shader::bindNames(const char *name,...) {
	Parameter parameter;
	getParameter(name,&parameter);
	parameters.append(parameter);
	va_list args;
	va_start(args,name);
	while(1) {
		const char *name = va_arg(args,const char*);
		if(name == NULL) break;
		getParameter(name,&parameter);
		parameters.append(parameter);
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
	if(vertex_id) glEnable(vertex_target);
	if(fragment_id) glEnable(fragment_target);
}

/*
 */
void Shader::disable() {
	if(program) glUseProgramObjectARB(0);
	if(vertex_id) glDisable(vertex_target);
	if(fragment_id) glDisable(fragment_target);
}

/*
 */
void Shader::bind() {
	if(program) glUseProgramObjectARB(program);
	if(vertex_id) {
		if(vertex_target == GL_VERTEX_PROGRAM_ARB) glBindProgramARB(vertex_target,vertex_id);
	}
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramARB(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(fragment_target,fragment_id);
	}
}

/*
 */
void Shader::bind(const float *value,...) {
	if(program == 0) {
		fprintf(stderr,"Shader::bind(): error GLSL shader isn't loaded\n");
		return;
	}
	glUseProgramObjectARB(program);
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramARB(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(fragment_target,fragment_id);
	}
	if(parameters[0].length == 1) glUniform1fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 2) glUniform2fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 3) glUniform3fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 4) glUniform4fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 9) glUniformMatrix3fvARB(parameters[0].location,1,false,value);
	else if(parameters[0].length == 16) glUniformMatrix4fvARB(parameters[0].location,1,false,value);
	va_list args;
	va_start(args,value);
	for(int i = 1; i < (int)parameters.size(); i++) {
		const float *value = va_arg(args,const float*);
		if(!value) break;
		if(parameters[i].length == 1) glUniform1fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 2) glUniform2fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 3) glUniform3fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 4) glUniform4fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 9) glUniformMatrix3fvARB(parameters[i].location,1,false,value);
		else if(parameters[i].length == 16) glUniformMatrix4fvARB(parameters[i].location,1,false,value);
	}
	va_end(args);
}

/*****************************************************************************/
/*                                                                           */
/* set parameters                                                            */
/*                                                                           */
/*****************************************************************************/

void Shader::setLocalParameter(int location,const float *value) {
	if(vertex_target == 0) {
		fprintf(stderr,"Shader::setLocalParameter(): error vertex program isn't loaded\n");
		return;
	}
	glProgramLocalParameter4fvARB(vertex_target,location,value);
}

/*
 */
void Shader::setParameter(const char *name,const float *value) {
	if(program == 0) {
		fprintf(stderr,"Shader::setLocalParameter(): error GLSL shader isn't loaded\n");
		return;
	}
	Parameter parameter;
	getParameter(name,&parameter);
	if(parameter.length == 1) glUniform1fvARB(parameter.location,1,value);
	else if(parameter.length == 2) glUniform2fvARB(parameter.location,1,value);
	else if(parameter.length == 3) glUniform3fvARB(parameter.location,1,value);
	else if(parameter.length == 4) glUniform4fvARB(parameter.location,1,value);
	else if(parameter.length == 9) glUniformMatrix3fvARB(parameter.location,1,false,value);
	else if(parameter.length == 16) glUniformMatrix4fvARB(parameter.location,1,false,value);
}

/*
 */
void Shader::setParameters(const float *value,...) {
	if(program == 0) {
		fprintf(stderr,"Shader::setLocalParameter(): error GLSL shader isn't loaded\n");
		return;
	}
	if(parameters[0].length == 1) glUniform1fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 2) glUniform2fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 3) glUniform3fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 4) glUniform4fvARB(parameters[0].location,1,value);
	else if(parameters[0].length == 9) glUniformMatrix3fvARB(parameters[0].location,1,false,value);
	else if(parameters[0].length == 16) glUniformMatrix4fvARB(parameters[0].location,1,false,value);
	va_list args;
	va_start(args,value);
	for(int i = 1; i < (int)parameters.size(); i++) {
		const float *value = va_arg(args,const float*);
		if(!value) break;
		if(parameters[i].length == 1) glUniform1fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 2) glUniform2fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 3) glUniform3fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 4) glUniform4fvARB(parameters[i].location,1,value);
		else if(parameters[i].length == 9) glUniformMatrix3fvARB(parameters[i].location,1,false,value);
		else if(parameters[i].length == 16) glUniformMatrix4fvARB(parameters[i].location,1,false,value);
	}
	va_end(args);
}
