/*	font for opengl
 *	need texture.h
 *
 *		written by Alexander Zaprjagaev
 *		frustum@public.tsu.ru
 */

#include "font.h"

/*
 */
font_t *font_load(char *name) {
	int i,j,k,x,y,size,step,space[256][2];
	unsigned char *data = NULL;
	font_t *font;
	char *ext = strrchr(name,'.');
	if(!ext) {
		fprintf(stderr,"unknown file format\n");
		return NULL;
	}
	if(!strcasecmp(ext,".tga")) data = texture_load_tga(name,&size,&size);
	else if(!strcasecmp(ext,".jpg")) data = texture_load_jpeg(name,&size,&size);
	if(!data) {
		fprintf(stderr,"error open \"%s\" file\n",name);
		return NULL;
	}
	font = calloc(1,sizeof(font_t));
	glGenTextures(1,&font->texture_id);
	glBindTexture(GL_TEXTURE_2D,font->texture_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,size,
		0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	step = size / 16;
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
	free(data);
	font->list_id = glGenLists(256);
	for(y = 0, i = 0; y < 16; y++) {
		for(x = 0; x < 16; x++, i++) {
			float s = (float)x / 16.0 + (float)space[i][0] / (float)size;
			float t = (float)y / 16.0;
			float ds = (float)space[i][1] / (float)size;
			float dt = 1.0 / 16.0;
			glNewList(font->list_id + i,GL_COMPILE);
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
	font->step = step;
	return font;
}

/*
 */
void font_printf(font_t *font,float width,float height,float x,float y,
	char *string,...) {
	int n;
	char *s,buf[1024];
	va_list argptr;
	va_start(argptr,string);
	vsprintf(buf,string,argptr);
	va_end(argptr);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,width,height,0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(x,y,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,font->texture_id);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	for(s = buf, n = 0; *s; s++) {
		if(*s == '\n') {
			n++;
			glLoadIdentity();
			glTranslatef(x,y + font->step * n,0);
		} else if(*s == '\r') {
			glLoadIdentity();
			glTranslatef(x,y + font->step * n,0);
		} else if(*s == '\t') {
			glTranslatef(font->step * 2,0,0);
		} else if(*s == ' ') {
			glTranslatef(font->step / 2,0,0);
		} else {
			glCallList(font->list_id + *s);
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
