/* Font
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
#include <stdarg.h>

#include "Image.h"
#include "Font.h"

#ifdef XPM_RESOURCES
#include "xpm/font.xpm"
#endif

/*****************************************************************************/
/*                                                                           */
/* Font                                                                      */
/*                                                                           */
/*****************************************************************************/

/*
 */
Font::Font(const char *name) {
	
	int width,height;
#ifdef XPM_RESOURCES
	unsigned char *data = Image::load_xpm(font_xpm,width,height);
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		unsigned char c = *(s + 0);
		unsigned char a = *(s + 1);
		*s++ = c;
		*s++ = c;
		*s++ = c;
		*s++ = a;
	}
#else
	unsigned char *data = Image::load(name,width,height);
#endif
	if(data == NULL) {
		fprintf(stderr,"Font::Font(): can't open \"%s\" file\n",name);
		return;
	}
	
	glGenTextures(1,&texture_id);
	glBindTexture(GL_TEXTURE_2D,texture_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	
	int size = width;
	step = size / 16;
	for(int y = 0, i = 0; y < 16; y++) {
		for(int x = 0; x < 16; x++, i++) {
			space[i][0] = 0;
			for(int j = 0; j < step; j++) {
				unsigned char *s = data + (size * y * step + x * step + j) * 4;
				int k;
				for(k = 0; k < step; k++) {
					if(*(s + 3) > 16) break;
					s += size * 4;
				}
				if(k != step) break;
				space[i][0]++;
			}
			space[i][1] = 0;
			if(space[i][0] == step) continue;
			for(int j = step - 1; j >= 0; j--) {
				unsigned char *s = data + (size * y * step + x * step + j) * 4;
				int k;
				for(k = 0; k < step; k++) {
					if(*(s + 3) > 16) break;
					s += size * 4;
				}
				if(k != step) break;
				space[i][1]++;
			}
			space[i][1] = step - space[i][0] - space[i][1] + 1;
		}
	}
	delete [] data;
	
	list_id = glGenLists(256);
	for(int y = 0, i = 0; y < 16; y++) {
		for(int x = 0; x < 16; x++, i++) {
			float s = (float)x / 16.0f + (float)space[i][0] / (float)size;
			float t = (float)y / 16.0f;
			float ds = (float)space[i][1] / (float)size;
			float dt = 1.0 / 16.0;
			glNewList(list_id + i,GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(s,t);
				glVertex2i(0,0);
				glTexCoord2f(s,t + dt);
				glVertex2i(0,step);
				glTexCoord2f(s + ds,t + dt);
				glVertex2i(space[i][1],step);
				glTexCoord2f(s + ds,t);
				glVertex2i(space[i][1],0);
			glEnd();
			glTranslatef((float)space[i][1],0,0);
			glEndList();
		}
	}
}

Font::~Font() {
	glDeleteTextures(1,&texture_id);
	glDeleteLists(256,list_id);
}

/*
 */
void Font::enable(int width,int height) {
	this->width = width;
	this->height = height;
	glGetFloatv(GL_PROJECTION_MATRIX,projection);
	glGetFloatv(GL_MODELVIEW_MATRIX,modelview);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,height,0,-1000.0f,1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_ALWAYS);
}

/*
 */
void Font::disable() {
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
}

/*
 */
int Font::getSize(char c) {
	if(c == 0) return step;
	if(c == '\t') return step * 2;
	if(c == ' ') return step / 2;
	return space[(unsigned char)c][1];
}

/*
 */
void Font::getSize(int font,const char *str,int &x,int &y) {
	x = 0;
	y = 0;
	int lines = 1;
	int length = 0;
	const char *s = str;
	while(*s) {
		if(*s == '\033' && *(s + 1) == '[') {
			while(*s && *s != 'm') s++;
		} else if(*s == '\n') {
			lines++;
			if(x < length) x = length;
			length = 0;
		} else if(*s == '\r') {
			if(x < length) x = length;
			length = 0;
		} else if(*s == '\t') {
			length += step * 2;
		} else if(*s == ' ') {
			length += step / 2;
		} else {
			length += getSize(*s);
		}
		s++;
	}
	if(x < length) x = length;
	y = step * lines;
	if(font != 0) {
		x = (int)((float)x * font / step);
		y = (int)((float)y * font / step);
	}
}

/*
 */
void Font::puts(int x,int y,int font,const char *str) {
	
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture_id);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
	
	float scale = 1.0f;
	if(font != 0) scale = (float)font / (float)step;
	glScalef(scale,scale,0);
	glTranslatef((float)x / scale,(float)y / scale,0);
	
	float color[4];
	glGetFloatv(GL_CURRENT_COLOR,color);
	
	int underline = 0;
	
	int lines = 0;
	const char *s = str;
	while(*s) {
		if(*s == '\033' && *(s + 1) == '[') {
			s += 2;
			int code;
			sscanf(s,"%dm",&code);
			if(code == 0) glColor3fv(color);
			else if(code == 4) underline = 1;
			else if(code == 24) underline = 0;
			else if(code == 30) glColor3f(0.28f,0.28f,0.28f);
			else if(code == 31) glColor3f(1,0,0);
			else if(code == 32) glColor3f(0,1,0);
			else if(code == 33) glColor3f(1,1,0);
			else if(code == 34) glColor3f(0,0,1);
			else if(code == 35) glColor3f(1,0,1);
			else if(code == 36) glColor3f(0,1,1);
			else if(code == 37) glColor3f(1,1,1);
			while(*s && *s != 'm') s++;
		} else if(*s == '\n') {
			lines++;
			glPopMatrix();
			glPushMatrix();
			glScalef(scale,scale,0);
			glTranslatef((float)x / scale,(float)((float)y / scale + step * lines),0);
		} else if(*s == '\r') {
			glPopMatrix();
			glPushMatrix();
			glScalef(scale,scale,0);
			glTranslatef((float)x / scale,(float)((float)y / scale + step * lines),0);
		} else if(*s == '\t') {
			glTranslatef((float)step * 2.0f,0,0);
			if(underline) {
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_QUADS);
				glVertex2i(-getSize(' ') * 2,step - step / 16);
				glVertex2i(-getSize(' ') * 2,step);
				glVertex2i(0,step);
				glVertex2i(0,step - step / 16);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}
		} else if(*s == ' ') {
			glTranslatef((float)step / 2.0f,0,0);
			if(underline) {
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_QUADS);
				glVertex2i(-getSize(' '),step - step / 16);
				glVertex2i(-getSize(' '),step);
				glVertex2i(0,step);
				glVertex2i(0,step - step / 16);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}
		} else {
			glCallList(list_id + *(const unsigned char*)s);
			if(underline) {
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_QUADS);
				glVertex2i(-getSize(*s),step - step / 16);
				glVertex2i(-getSize(*s),step);
				glVertex2i(0,step);
				glVertex2i(0,step - step / 16);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}
		}
		s++;
	}
	
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

/*
 */
void Font::printf(int x,int y,int font,const char *format,...) {
	char buf[4096];
	va_list argptr;
	va_start(argptr,format);
	vsnprintf(buf,sizeof(buf),format,argptr);
	va_end(argptr);
	puts(x,y,font,buf);
}
