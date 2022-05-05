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

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

/*
 */
class Texture {
	
	public:
		
		enum {
			TEXTURE_2D = GL_TEXTURE_2D,
			TEXTURE_RECT = GL_TEXTURE_RECTANGLE_ARB,
			TEXTURE_CUBE = GL_TEXTURE_CUBE_MAP_ARB,
			TEXTURE_3D = GL_TEXTURE_3D,
		};
		
		enum {
			// format
			LUMINANCE = 1 << 0,
			LUMINANCE_ALPHA = 1 << 1,
			RGB = 1 << 2,
			RGBA = 1 << 3,
			DEPTH = 1 << 4,
			
			FORMAT_MASK = (LUMINANCE | LUMINANCE_ALPHA | RGB | RGBA | DEPTH),
			
			// flags
			FLOAT_16 = 1 << 5,
			FLOAT_32 = 1 << 6,
			
			FLOAT_MASK = (FLOAT_16 | FLOAT_32),
			
			// compression
			DXT = 1 << 7,
			
			COMPRESSION_MASK = (DXT),
			
			// wrapping
			REPEAT = 1 << 8,
			CLAMP = 1 << 9,
			CLAMP_TO_EDGE = 1 << 10,
			
			WRAP_MASK = (REPEAT | CLAMP | CLAMP_TO_EDGE),
			
			// filter
			NEAREST = 1 << 11,
			LINEAR = 1 << 12,
			NEAREST_MIPMAP_NEAREST = 1 << 13,
			LINEAR_MIPMAP_NEAREST = 1 << 14,
			LINEAR_MIPMAP_LINEAR = 1 << 15,
			
			FILTER_MASK = (NEAREST | LINEAR | NEAREST_MIPMAP_NEAREST | LINEAR_MIPMAP_NEAREST | LINEAR_MIPMAP_LINEAR),
			
			// anisotropy
			ANISOTROPY_1 = 1 << 16,
			ANISOTROPY_2 = 1 << 17,
			ANISOTROPY_4 = 1 << 18,
			ANISOTROPY_8 = 1 << 19,
			ANISOTROPY_16 = 1 << 20,
			
			ANISOTROPY_MASK = (ANISOTROPY_1 | ANISOTROPY_2 | ANISOTROPY_4 | ANISOTROPY_8 | ANISOTROPY_16),
		};
	
		Texture(int width,int height,GLuint target = TEXTURE_2D,int flags = RGBA | LINEAR_MIPMAP_LINEAR);
		Texture(const char *name,GLuint target = TEXTURE_2D,int flags = RGBA | LINEAR_MIPMAP_LINEAR);
		virtual ~Texture();
		
		void load(const char *name,GLuint target,int flags);
		void setFlags(int flags);
		
		void enable(Texture *old = NULL);
		void enable(int unit);
		
		void enable(int unit,Texture *old);
		void disable();
		void disable(int unit);
		
		void bind(Texture *old = NULL);
		void bind(int unit);
		void bind(int unit,Texture *old);
		
		void unbind();
		void unbind(int unit);
		
		void copy(GLuint target = 0);
		void render(float x0 = -1.0,float y0 = -1.0,float x1 = 1.0,float y1 = 1.0);
		
		void texImage2D(const unsigned char *data,int width,int height);
		
		int width;
		int height;
		int depth;
		GLuint target;
		int flags;
		
		GLuint type;
		GLuint internalformat;
		GLuint format;
		
		GLuint texture_id;
};

#endif /* __TEXTURE_H__ */
