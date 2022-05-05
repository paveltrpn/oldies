/*	texture
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

class Texture {
public:
	enum {
		TEXTURE_1D = GL_TEXTURE_1D,
		TEXTURE_2D = GL_TEXTURE_2D,
		TEXTURE_RECT = GL_TEXTURE_RECTANGLE_NV,
		TEXTURE_CUBE = GL_TEXTURE_CUBE_MAP
	};
	enum {
		NEAREST = 1 << 1,
		LINEAR = 1 << 2,
		TRILINEAR = 1 << 3,
		MIPMAP_SGIS = 1 << 4,
		CLAMP = 1 << 5,
		CLAMP_TO_EDGE = 1 << 6,
		FLOAT = 1 << 7
	};

	Texture(int width,int height,GLuint target = TEXTURE_2D,int flag = LINEAR);
	Texture(const char *name,GLuint target = TEXTURE_2D,int flag = TRILINEAR | MIPMAP_SGIS);
	~Texture();
	
	void enable();
	void disable();
	void bind();
	void copy(GLuint t = 0);
	void render(float x0 = -1.0,float y0 = -1.0,float x1 = 1.0,float y1 = 1.0);
	
	static unsigned char *load(const char *name,int *width,int *height);
	static unsigned char *load_tga(const char *name,int *width,int *height);
	static unsigned char *load_png(const char *name,int *width,int *height);
	static unsigned char *load_jpeg(const char *name,int *width,int *height);
	static unsigned char *load_dds(const char *name,int *width,int *height);
	
	static int save_tga(const char *name,const unsigned char *data,int width,int height);
	static int save_jpeg(const char *name,const unsigned char *data,int width,int height,int quality);
	
	int width,height;
	GLenum target;
	GLuint id;
};

#endif /* __TEXTURE_H__ */
