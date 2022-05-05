/* Texture
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

#include <stdio.h>

#include "GLExt.h"
#include "Image.h"
#include "Texture.h"

/*****************************************************************************/
/*                                                                           */
/* Texture                                                                   */
/*                                                                           */
/*****************************************************************************/

/*
 */
Texture::Texture(int width,int height,GLuint target,int flags) {
	
	this->width = width;
	this->height = height;
	depth = 1;
	this->target = target;
	this->flags = flags;
	
	glGenTextures(1,&texture_id);
	glBindTexture(target,texture_id);
	
	// format
	if(flags & LUMINANCE) format = GL_LUMINANCE;
	else if(flags & LUMINANCE_ALPHA) format = GL_LUMINANCE_ALPHA;
	else if(flags & RGB) format = GL_RGB;
	else if(flags & RGBA) format = GL_RGBA;
	else if(flags & DEPTH) format = GL_DEPTH_COMPONENT;
	else {
		fprintf(stderr,"Texture::Texture(): unknown texture format\n");
		return;
	}
	
	type = GL_UNSIGNED_BYTE;
	internalformat = format;
	
	if(flags & DEPTH) {
		type = GL_UNSIGNED_INT;
		GLint depth;
		glGetIntegerv(GL_DEPTH_BITS,&depth);
		if(depth == 16) internalformat = GL_DEPTH_COMPONENT16;
		else internalformat = GL_DEPTH_COMPONENT24;
	}
	
	// float texture
	if(flags & FLOAT_16) {
		type = GL_FLOAT;
		if(flags & LUMINANCE) internalformat = GL_LUMINANCE_FLOAT16_ATI;
		else if(flags & LUMINANCE_ALPHA) internalformat = GL_LUMINANCE_ALPHA_FLOAT16_ATI;
		else if(flags & RGB) internalformat = GL_RGB_FLOAT16_ATI;
		else if(flags & RGBA) internalformat = GL_RGBA_FLOAT16_ATI;
		else {
			fprintf(stderr,"Texture::Texture(): FLOAT_16 flags is accessible only for LUMINANCE, LUMINANCE_ALPHA, RGB or RGBA formats\n");
			return;
		}
	} else if(flags & FLOAT_32) {
		type = GL_FLOAT;
		if(flags & LUMINANCE) internalformat = GL_LUMINANCE_FLOAT32_ATI;
		else if(flags & LUMINANCE_ALPHA) internalformat = GL_LUMINANCE_ALPHA_FLOAT32_ATI;
		else if(flags & RGB) internalformat = GL_RGB_FLOAT32_ATI;
		else if(flags & RGBA) internalformat = GL_RGBA_FLOAT32_ATI;
		else {
			fprintf(stderr,"Texture::Texture(): FLOAT_32 flags is accessible only for LUMINANCE, RGB or RGBA formats\n");
			return;
		}
	}
	
	// compression
	if(flags & DXT) {
		if(flags & RGB) internalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		else if(flags & RGBA) internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	}
	
	// wrapping
	if(flags & REPEAT) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_REPEAT);
	} else if(flags & CLAMP) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP);
	} else if(flags & CLAMP_TO_EDGE) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	}
	
	// filter
	if(flags & NEAREST) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	} else if(flags & LINEAR) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	} else if(flags & NEAREST_MIPMAP_NEAREST){
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_LINEAR) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	}
	
	// anisotropy
	if(flags & ANISOTROPY_1) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0);
	else if(flags & ANISOTROPY_2) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,2.0);
	else if(flags & ANISOTROPY_4) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,4.0);
	else if(flags & ANISOTROPY_8) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,8.0);
	else if(flags & ANISOTROPY_16) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,16.0);
	
	if(target == TEXTURE_2D) glTexImage2D(target,0,internalformat,width,height,0,format,type,NULL);
	else if(target == TEXTURE_RECT) glTexImage2D(target,0,internalformat,width,height,0,format,type,NULL);
	else if(target == TEXTURE_CUBE) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,0,internalformat,width,height,0,format,type,NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,0,internalformat,width,height,0,format,type,NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,0,internalformat,width,height,0,format,type,NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,0,internalformat,width,height,0,format,type,NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,0,internalformat,width,height,0,format,type,NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,0,internalformat,width,height,0,format,type,NULL);
	}
	else if(target == TEXTURE_3D) glTexImage3D(target,0,internalformat,width,height,depth,0,format,type,NULL);
}

/*
 */
Texture::Texture(const char *name,GLuint target,int flags) {
	load(name,target,flags);
}

/*
 */
Texture::~Texture() {
	glDeleteTextures(1,&texture_id);
}

/*
 */
void Texture::load(const char *name,GLuint target,int flags) {
	
	width = 0;
	height = 0;
	depth = 1;
	this->target = target;
	this->flags = flags;
	
	glGenTextures(1,&texture_id);
	glBindTexture(target,texture_id);
	
	// format
	if(flags & LUMINANCE) format = GL_LUMINANCE;
	else if(flags & LUMINANCE_ALPHA) format = GL_LUMINANCE_ALPHA;
	else if(flags & RGB) format = GL_RGB;
	else if(flags & RGBA) format = GL_RGBA;
	else if(flags & DEPTH) {
		fprintf(stderr,"Texture::Texture(): can't create DEPTH texture from \"%s\" file\n",name);
		return;
	} else {
		fprintf(stderr,"Texture::Texture(): unknown texture format\n");
		return;
	}
	
	type = GL_UNSIGNED_BYTE;
	internalformat = format;
	
	// float texture
	if(flags & FLOAT_16) {
		type = GL_FLOAT;
		if(flags & LUMINANCE) internalformat = GL_LUMINANCE_FLOAT16_ATI;
		else if(flags & LUMINANCE_ALPHA) internalformat = GL_LUMINANCE_ALPHA_FLOAT16_ATI;
		else if(flags & RGB) internalformat = GL_RGB_FLOAT16_ATI;
		else if(flags & RGBA) internalformat = GL_RGBA_FLOAT16_ATI;
		else {
			fprintf(stderr,"Texture::Texture(): FLOAT_16 flags is accessible only for LUMINANCE, LUMINANCE_ALPHA, RGB or RGBA formats\n");
			return;
		}
	}
	else if(flags & FLOAT_32) {
		type = GL_FLOAT;
		if(flags & LUMINANCE) internalformat = GL_LUMINANCE_FLOAT32_ATI;
		else if(flags & LUMINANCE_ALPHA) internalformat = GL_LUMINANCE_ALPHA_FLOAT32_ATI;
		else if(flags & RGB) internalformat = GL_RGB_FLOAT32_ATI;
		else if(flags & RGBA) internalformat = GL_RGBA_FLOAT32_ATI;
		else {
			fprintf(stderr,"Texture::Texture(): FLOAT_32 flags is accessible only for LUMINANCE, RGB or RGBA formats\n");
			return;
		}
	}
	
	// compresson
	if(flags & DXT) {
		if(flags & RGB) internalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		else if(flags & RGBA) internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	}
	
	// wrapping
	if(flags & REPEAT) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_REPEAT);
	} else if(flags & CLAMP) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP);
	} else if(flags & CLAMP_TO_EDGE) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	}
	
	// filter
	if(flags & NEAREST) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	} else if(flags & LINEAR) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	} else if(flags & NEAREST_MIPMAP_NEAREST){
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_LINEAR) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	}
	
	// anisotropy
	if(flags & ANISOTROPY_1) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0);
	else if(flags & ANISOTROPY_2) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,2.0);
	else if(flags & ANISOTROPY_4) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,4.0);
	else if(flags & ANISOTROPY_8) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,8.0);
	else if(flags & ANISOTROPY_16) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,16.0);
	
	if(target == TEXTURE_2D || target == TEXTURE_RECT) {
		unsigned char *data = Image::load(name,width,height);
		if(data) {
			if((flags & FLOAT_16) || (flags & FLOAT_32)) {
				float *data_float = NULL;
				if(format == GL_LUMINANCE) data_float = Image::rgba2luminance_float(data,width,height);
				else if(format == GL_LUMINANCE_ALPHA) data_float = Image::rgba2luminance_alpha_float(data,width,height);
				else if(format == GL_RGB) data_float = Image::rgba2rgb_float(data,width,height);
				else if(format == GL_RGBA) data_float = Image::rgba2rgba_float(data,width,height);
				glTexImage2D(target,0,internalformat,width,height,0,format,type,data_float);
				if(data_float) delete [] data_float;
			}
			else {
				if(format == GL_LUMINANCE) data = Image::rgba2luminance(data,width,height);
				else if(format == GL_LUMINANCE_ALPHA) data = Image::rgba2luminance_alpha(data,width,height);
				else if(format == GL_RGB) data = Image::rgba2rgb(data,width,height);
				glTexImage2D(target,0,internalformat,width,height,0,format,type,data);
				delete [] data;
			}
		}
	}
	else if(target == TEXTURE_CUBE) {
		GLuint targets[6] = {
			GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};
		char *suffix[6] = { "px", "nx", "py", "ny", "pz", "nz" };
		for(int i = 0; i < 6; i++) {
			char buf[1024];
			sprintf(buf,name,suffix[i]);
			unsigned char *data = Image::load(buf,width,height);
			if(data) {
				if((flags & FLOAT_16) || (flags & FLOAT_32)) {
					float *data_float = NULL;
					if(format == GL_LUMINANCE) data_float = Image::rgba2luminance_float(data,width,height);
					else if(format == GL_LUMINANCE_ALPHA) data_float = Image::rgba2luminance_alpha_float(data,width,height);
					else if(format == GL_RGB) data_float = Image::rgba2rgb_float(data,width,height);
					else if(format == GL_RGBA) data_float = Image::rgba2rgba_float(data,width,height);
					glTexImage2D(targets[i],0,internalformat,width,height,0,format,type,data_float);
					if(data_float) delete [] data_float;
				}
				else {
					if(format == GL_LUMINANCE) data = Image::rgba2luminance(data,width,height);
					else if(format == GL_LUMINANCE_ALPHA) data = Image::rgba2luminance_alpha(data,width,height);
					else if(format == GL_RGB) data = Image::rgba2rgb(data,width,height);
					glTexImage2D(targets[i],0,internalformat,width,height,0,format,type,data);
					delete [] data;
				}
			}
		}
	}
	else if(target == TEXTURE_3D) {
		unsigned char *data = Image::load_3d(name,width,height,depth);
		format = GL_RGBA;
		glTexImage3D(target,0,internalformat,width,height,depth,0,format,GL_UNSIGNED_BYTE,data);
		if(data) delete [] data;
	}
}

/*
 */
void Texture::setFlags(int flags) {
	
	if((flags & FORMAT_MASK) != 0 || (flags & FLOAT_MASK) != 0) {
		fprintf(stderr,"Texture::setFlags(): can't change texture format\n");
		return;
	}
	if((flags & COMPRESSION_MASK) != 0) {
		fprintf(stderr,"Texture::setFlags(): can't change texture compression\n");
		return;
	}
	
	this->flags &= ~WRAP_MASK;
	this->flags &= ~FILTER_MASK;
	this->flags &= ~ANISOTROPY_MASK;
	this->flags |= flags;
	
	glBindTexture(target,texture_id);
	
	// wrapping
	if(flags & REPEAT) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_REPEAT);
	} else if(flags & CLAMP) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP);
	} else if(flags & CLAMP_TO_EDGE) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	}
	
	// filter
	if(flags & NEAREST) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	} else if(flags & LINEAR) {
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	} else if(flags & NEAREST_MIPMAP_NEAREST){
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	} else if(flags & LINEAR_MIPMAP_LINEAR) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	}
	
	// anisotropy
	if(flags & ANISOTROPY_1) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0);
	else if(flags & ANISOTROPY_2) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,2.0);
	else if(flags & ANISOTROPY_4) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,4.0);
	else if(flags & ANISOTROPY_8) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,8.0);
	else if(flags & ANISOTROPY_16) glTexParameterf(target,GL_TEXTURE_MAX_ANISOTROPY_EXT,16.0);
}

/*
 */
void Texture::enable(Texture *old) {
	if(old) {
		if(old == this) return;
		if(old->target != target) {
			glDisable(old->target);
			glEnable(target);
		}
	} else {
		glEnable(target);
	}
}

void Texture::enable(int unit) {
	enable(unit,NULL);
}

void Texture::enable(int unit,Texture *old) {
	if(old == this) return;
	if(unit != 0) glActiveTexture(GL_TEXTURE0 + unit);
	enable(old);
	if(unit != 0) glActiveTexture(GL_TEXTURE0);
}

/*
 */
void Texture::disable() {
	glDisable(target);
}

void Texture::disable(int unit) {
	if(unit != 0) glActiveTexture(GL_TEXTURE0 + unit);
	disable();
	if(unit != 0) glActiveTexture(GL_TEXTURE0);
}

/*
 */
void Texture::bind(Texture *old) {
	if(old == this) return;
	glBindTexture(target,texture_id);
}

void Texture::bind(int unit) {
	bind(unit,NULL);
}

void Texture::bind(int unit,Texture *old) {
	if(old == this) return;
	if(unit != 0) glActiveTexture(GL_TEXTURE0 + unit);
	bind(old);
	if(unit != 0) glActiveTexture(GL_TEXTURE0);
}

/*
 */
void Texture::unbind() {
	glBindTexture(target,0);
}

void Texture::unbind(int unit) {
	if(unit != 0) glActiveTexture(GL_TEXTURE0 + unit);
	unbind();
	if(unit != 0) glActiveTexture(GL_TEXTURE0);
}

/*
 */
void Texture::copy(GLuint target) {
	if(target == 0) glCopyTexSubImage2D(this->target,0,0,0,0,0,width,height);
	else glCopyTexSubImage2D(target,0,0,0,0,0,width,height);
}

/*
 */
void Texture::render(float x0,float y0,float x1,float y1) {
	glBegin(GL_QUADS);
	if(target == TEXTURE_RECT) {
		glTexCoord2i(0,0);
		glVertex2f(x0,y0);
		glTexCoord2i(width,0);
		glVertex2f(x1,y0);
		glTexCoord2i(width,height);
		glVertex2f(x1,y1);
		glTexCoord2i(0,height);
		glVertex2f(x0,y1);
	} else {
		glTexCoord2f(0,0);
		glVertex2f(x0,y0);
		glTexCoord2f(1,0);
		glVertex2f(x1,y0);
		glTexCoord2f(1,1);
		glVertex2f(x1,y1);
		glTexCoord2f(0,1);
		glVertex2f(x0,y1);
	}
	glEnd();
}

/*
 */
void Texture::texImage2D(const unsigned char *data,int width,int height) {
	glBindTexture(target,texture_id);
	glTexImage2D(target,0,internalformat,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
}
