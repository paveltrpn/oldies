/*	font for opengl
 *	need texture.h
 *
 *		written by Alexander Zaprjagaev
 *		frustum@public.tsu.ru
 */

#ifndef __FONT_H__
#define __FONT_H__

#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#ifdef _WIN32
#include "windows.h"
#endif
#include <GL/gl.h>
#include "texture.h"

typedef struct {
	int texture_id;
	int list_id;
	int step;
} font_t;

font_t *font_load(char *name);
void font_printf(font_t *font,float width,float height,float x,float y,char *string,...);

#endif /* __FONT_H__ */
