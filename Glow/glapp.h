/*	OpenGL App
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef __GLAPP_H__
#define __GLAPP_H__

class GLApp {
public:
	GLApp();
	virtual ~GLApp() { }
	
	int setVideoMode(int w,int h,int fs);
	int setTitle(const char *title);
	int setPointer(int x,int y);
	int showPointer(int show);
	int error();
	int main();
	int exit() { done = 1; return 0; }
	
	virtual int idle() { return 0; }
	virtual int render() { return 0; }
	
	enum {
		KEY_ESC = 256,
		KEY_RETURN,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN
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
	int fullScreen;
	
	int mouseX;
	int mouseY;
	int mouseButton;
	
	float fps;
	float ifps;
};

#endif /* __GLAPP_H__ */
