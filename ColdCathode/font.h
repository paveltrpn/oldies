/*	font
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __FONT_H__
#define __FONT_H__

#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "texture.h"

class Font {
public:
	Font();
	Font(const char *name);
	~Font();
	
	void load(const char *name);
	void puts(float x,float y,const char *str);
	void printf(float width,float height,float x,float y,const char *str,...);
	void printf_center(float width,float height,float x,float y,const char *str,...);
	int getStep() { return step; }

protected:
	GLuint texture_id;
	GLuint list_id;
	int step;
	int space[256][2];
};

#endif /* __FONT_H__ */
