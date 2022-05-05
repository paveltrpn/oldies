/*	texture
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <setjmp.h>

extern "C" {
#include <jpeglib.h>
}

extern unsigned char *texture_load_tga(const char *name,int *width,int *height);
extern unsigned char *texture_load_jpeg(const char *name,int *width,int *height);
extern int texture_save_tga(const char *name,unsigned char *data,int width,int height);
extern int texture_save_jpeg(const char *name,unsigned char *data,int width,int height,int quality);

class Texture {
public:
	enum {
		TRILINEAR = 1 << 1,
		LINEAR = 1 << 2,
		MIPMAP_SGIS = 1 << 3,
		CLAMP = 1 << 4,
		CLAMP_TO_EDGE = 1 << 5
	};
	enum {
		TEXTURE_1D = GL_TEXTURE_1D,
		TEXTURE_2D = GL_TEXTURE_2D
	};

	Texture();
	Texture(const char *name,int flag = TRILINEAR | MIPMAP_SGIS,int target = TEXTURE_2D);
	~Texture();
	
	void load(const char *name,int flag = TRILINEAR | MIPMAP_SGIS,int target = TEXTURE_2D);
	
	void enable();
	void disable();
	void bind();

	GLenum texture_target;
	GLuint texture_id;
};

#endif /* __TEXTURE_H__ */
