/* OpenGL App
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
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

#ifndef __GLAPP_H__
#define __GLAPP_H__

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

class GLApp {
public:
	
	GLApp();
	virtual ~GLApp();
	
	enum {
		FULLSCREEN = 1 << 0,
		MULTISAMPLE_2 = 1 << 1,
		MULTISAMPLE_4 = 1 << 2,
	};
	
	int setVideoMode(int w,int h,int flags = 0);
	void setTitle(const char *title);
	
	void setCursor(int x,int y);
	void showCursor(int show);
	
	int checkExtension(const char *extension);
	
	void error();
	void exit(const char *error = NULL,...);
	
	int selectFile(const char *title,char *name);
	
	void update();
	void swap();
	
	void main();
	
	virtual void idle() { }
	virtual void render() { }
	virtual void buttonPress(int) { }
	virtual void buttonRelease(int) { }
	virtual void keyPress(int) { }
	virtual void keyRelease(int) { }
	
	enum {
		KEY_ESC = 256,
		KEY_TAB,
		KEY_RETURN,
		KEY_BACKSPACE,
		KEY_DELETE,
		KEY_HOME,
		KEY_END,
		KEY_PGUP,
		KEY_PGDOWN,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_SHIFT,
		KEY_CTRL,
		KEY_ALT,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
	};
	
	enum {
		BUTTON_LEFT = 1 << 0,
		BUTTON_MIDDLE = 1 << 1,
		BUTTON_RIGHT = 1 << 2,
		BUTTON_UP = 1 << 3,
		BUTTON_DOWN = 1 << 4
	};
	
	int done;
	int keys[512];
	
	int windowWidth;
	int windowHeight;
	int flags;
	char title[1024];
	
	int mouseX;
	int mouseY;
	int mouseButton;
	
	float fps;
	float ifps;
};

#endif /* __GLAPP_H__ */
