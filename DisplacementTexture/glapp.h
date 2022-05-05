/*	OpenGL App
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __GLAPP_H__
#define __GLAPP_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

class GLApp {
public:
	GLApp();
	virtual ~GLApp() { }
	
	int setVideoMode(int w,int h,int fs);
	int setTitle(const char *title);
	int setPointer(int x,int y);
	int showPointer(int show);
	int checkExtension(const char *extension);
	int error();
	static int exit(const char *error = NULL,...);
	int main();

	virtual int idle() { return 0; }
	virtual int render() { return 0; }
	
	virtual int buttonPress(int) { return 0; }
	virtual int buttonRelease(int) { return 0; }
	virtual int keyPress(int) { return 0; }
	virtual int keyRelease(int) { return 0; }
	
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
		KEY_CTRL
	};
	
	enum {
		BUTTON_LEFT = 1 << 0,
		BUTTON_MIDDLE = 1 << 1,
		BUTTON_RIGHT = 1 << 2,
		BUTTON_UP = 1 << 3,
		BUTTON_DOWN = 1 << 4
	};
	
	int keys[512];
	
	int windowWidth;
	int windowHeight;
	int fullScreen;
	
	int mouseX;
	int mouseY;
	int mouseButton;
	
	float fps;
	float ifps;
};

#endif /* __GLAPP_H__ */
