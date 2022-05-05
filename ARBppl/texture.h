/*	texture for opengl
 *	support tga and jpeg images
 *
 *		written by Alexander Zaprjagaev
 *		frustum@public.tsu.ru
 */

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <jpeglib.h>
#include <setjmp.h>

enum {
	TEXTURE_CLAMP = 1 << 0,
	TEXTURE_TRILINEAR = 1 << 1,
	TEXTURE_MIPMAP_SGIS = 1 << 2
};

unsigned char *texture_load_tga(char *name,int *width,int *height);
int texture_save_tga(char *name,unsigned char *data,int width,int height);
unsigned char *texture_load_jpeg(char *name,int *width,int *height);
int texture_save_jpeg(char *name,unsigned char *data,int width,int height,int quality);
int texture_load(char *name,int mode);
void texture_free(int id);

#endif /* __TEXTURE_H__ */
