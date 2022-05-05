/*	font
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#include "font.h"

Font::Font(const char *name) {
	memset(this,0,sizeof(Font));
	load(name);
}

Font::Font() {
	memset(this,0,sizeof(Font));
}

Font::~Font() {

}

/*
 */
void Font::load(const char *name) {
	int size;
	unsigned char *data = Texture::load(name,&size,&size);
	if(!data) return;
	glGenTextures(1,&texture_id);
	glBindTexture(GL_TEXTURE_2D,texture_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	step = size / 16;
	int x,y,i,j,k;
	for(y = 0, i = 0; y < 16; y++) {
		for(x = 0; x < 16; x++, i++) {
			unsigned char *ptr;
			space[i][0] = 0;
			for(j = 0; j < step; j++) {
				ptr = data + (size * y * step + x * step + j) * 4;
				for(k = 0; k < step; k++) {
					if(*(ptr + 3) != 0) break;
					ptr += size * 4;
				}
				if(k != step) break;
				space[i][0]++;
			}
			space[i][1] = 0;
			if(space[i][0] == step) continue;
			for(j = step - 1; j >= 0; j--) {
				ptr = data + (size * y * step + x * step + j) * 4;
				for(k = 0; k < step; k++) {
					if(*(ptr + 3) != 0) break;
					ptr += size * 4;
				}
				if(k != step) break;
				space[i][1]++;
			}
			space[i][1] = step - space[i][0] - space[i][1];
		}
	}
	delete data;
	list_id = glGenLists(256);
	for(y = 0, i = 0; y < 16; y++) {
		for(x = 0; x < 16; x++, i++) {
			float s = (float)x / 16.0 + (float)space[i][0] / (float)size;
			float t = (float)y / 16.0;
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
			glTranslatef(space[i][1],0,0);
			glEndList();
		}
	}
}

/*
 */
void Font::puts(float x,float y,const char *str) {
	char *s = (char*)str;
	glPushMatrix();
	glTranslatef(x,y,0);
	for(int n = 0; *s; s++) {
		if(*s == '\n') {
			n++;
			glLoadIdentity();
			glTranslatef(x,y + step * n,0);
		} else if(*s == '\r') {
			glLoadIdentity();
			glTranslatef(x,y + step * n,0);
		} else if(*s == '\t') {
			glTranslatef(step * 2,0,0);
		} else if(*s == ' ') {
			glTranslatef(step / 2,0,0);
		} else {
			glCallList(list_id + *(unsigned char*)s);
		}
	}
	glPopMatrix();
}

/*
 */
void Font::printf(float width,float height,float x,float y,const char *str,...) {
	char buf[1024];
	va_list argptr;
	va_start(argptr,str);
	vsprintf(buf,str,argptr);
	va_end(argptr);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,width,height,0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture_id);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_ALPHA);
	puts(x,y,buf);
	glBlendFunc(GL_ONE,GL_ONE);
	puts(x,y,buf);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

/*
 */
void Font::printf_center(float width,float height,float x,float y,const char *str,...) {
	char buf[1024];
	va_list argptr;
	va_start(argptr,str);
	vsprintf(buf,str,argptr);
	va_end(argptr);
	int length = 0;
	int max_length = 0;
	char *s = buf;
	for(; *s; s++) {
		if(*s != '\n') length += space[*(unsigned char*)s][1];
		else if(length > max_length) max_length = length;
	}
	if(length > max_length) max_length = length;
	printf(width,height,x - max_length / 2,y,"%s",buf);
}
