/*	OpenGL App
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef _WIN32

/*	linux
 */
#include <iostream>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <string.h>
#include "glapp.h"

static Display *display;
static int screen;
static Window window,rootWindow;
static Atom WM_DELETE_WINDOW;
static XF86VidModeModeInfo **modes;
static GLXContext glxContext;

/*
 */
GLApp::GLApp() {
	memset(this,0,sizeof(GLApp));
	display = XOpenDisplay(NULL);
	if(!display) {
		std::cerr << "couldn`t open display\n";
		done = 1;
	}
}

/*
 */
static int modescmp(const void *pa,const void *pb) {
	XF86VidModeModeInfo *a = *(XF86VidModeModeInfo**)pa;
	XF86VidModeModeInfo *b = *(XF86VidModeModeInfo**)pb;
	if(a->hdisplay > b->hdisplay) return -1;
	return b->vdisplay - a->vdisplay;
}

/*
 */
int GLApp::setVideoMode(int w,int h,int fs) {
	
	if(done || screen) return 0;
	
	windowWidth = w;
	windowHeight = h;
	fullScreen = fs;
	
	int attrib[] = { GLX_RGBA,GLX_RED_SIZE,1,GLX_GREEN_SIZE,1,GLX_BLUE_SIZE,1,GLX_DOUBLEBUFFER,GLX_DEPTH_SIZE,1,None };
	screen = DefaultScreen(display);
	rootWindow = RootWindow(display,screen);
	XVisualInfo *visualInfo = glXChooseVisual(display,screen,attrib);
	if(!visualInfo) {
		std::cerr << "couldn`t get an RGB, double-buffered visual\n";
		done = 1;
		return -1;
	}
	
	if(fullScreen) {
		int i,nmodes;
		XF86VidModeModeLine mode;
		if(XF86VidModeGetModeLine(display,screen,&nmodes,&mode) && XF86VidModeGetAllModeLines(display,screen,&nmodes,&modes)) {
			qsort(modes,nmodes,sizeof(XF86VidModeModeInfo*),modescmp);
			for(i = nmodes - 1; i > 0; i--) if (modes[i]->hdisplay >= windowWidth && modes[i]->vdisplay >= windowHeight) break;
			if(modes[i]->hdisplay != mode.hdisplay || modes[i]->vdisplay != mode.vdisplay) XF86VidModeSwitchToMode(display,screen,modes[i]);
			XF86VidModeSetViewPort(display,screen,0,0);
		} else fullScreen = 0;
	}
		
	XSetWindowAttributes attr;
	unsigned long mask;
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(display,rootWindow,visualInfo->visual,AllocNone);
	attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	if(fullScreen) {
		mask = CWBackPixel | CWColormap | CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWEventMask;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	
	window = XCreateWindow(display,rootWindow,0,0,windowWidth,windowHeight,0,visualInfo->depth,InputOutput,visualInfo->visual,mask,&attr);
	XMapWindow(display,window);
	
	if(fullScreen) {
		XMoveWindow(display,window,0,0);
		XRaiseWindow(display,window);
		XWarpPointer(display,None,window,0,0,0,0,windowWidth / 2,windowHeight / 2);
		XFlush(display);
		XF86VidModeSetViewPort(display,screen,0,0);
		XGrabPointer(display,window,True,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
		XGrabKeyboard(display,window,True,GrabModeAsync,GrabModeAsync,CurrentTime);
	} else {
		WM_DELETE_WINDOW = XInternAtom(display,"WM_DELETE_WINDOW",False);
		XSetWMProtocols(display,window,&WM_DELETE_WINDOW,1);
	}
	XFlush(display);
	
	glxContext = glXCreateContext(display,visualInfo,NULL,True);
	if(!glxContext) {
		std::cerr << "glXCreateContext failed\n";
		done = 1;
		return -1;
	}
	glXMakeCurrent(display,window,glxContext);
	
	return 0;
}

/*
 */
int GLApp::setTitle(const char *title) {
	XStoreName(display,window,title);
	XSetIconName(display,window,title);
	return 0;
}

/*
 */
int GLApp::setPointer(int x,int y) {
	XWarpPointer(display,None,window,0,0,0,0,x,y);
	XFlush(display);
	return 0;
}

/*
 */
int GLApp::showPointer(int show) {
	if(show) XDefineCursor(display,window,0);
	else {
		char data[] = { 0 };
		Cursor cursor;
		Pixmap blank;
		XColor dummy;
		blank = XCreateBitmapFromData(display,window,data,1,1);
		cursor = XCreatePixmapCursor(display,blank,blank,&dummy,&dummy,0,0);
		XDefineCursor(display,window,cursor);
		XFreePixmap(display,blank);
		XFreeCursor(display,cursor);
	}
	return 0;
}

/*
 */
static int getTime() {
	struct timeval tval;
	struct timezone tzone;
	gettimeofday(&tval,&tzone);
	return tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

/*
 */
int GLApp::main() {
	int key;
	int startTime = getTime(),endTime = 0,counter = 0;
    fps = 60;
	while(!done) {
		while(XPending(display) > 0) {
			XEvent event;
			XNextEvent(display,&event);
			switch(event.type) {
				case ClientMessage:
					if(event.xclient.format == 32 && event.xclient.data.l[0] == (long)WM_DELETE_WINDOW) done = 1;
					break;
				case ConfigureNotify:
					windowWidth = event.xconfigure.width;
					windowHeight = event.xconfigure.height;
					glViewport(0,0,windowWidth,windowHeight);
					break;
				case KeyPress:
					key = XLookupKeysym(&event.xkey,0);
					switch(key) {
						case XK_Escape: keys[KEY_ESC] = 1; break;
						case XK_Return: keys[KEY_RETURN] = 1; break;
						case XK_Left: keys[KEY_LEFT] = 1; break;
						case XK_Right: keys[KEY_RIGHT] = 1; break;
						case XK_Up: keys[KEY_UP] = 1; break;
						case XK_Down: keys[KEY_DOWN] = 1; break;
					}
					if(key < 256) keys[key] = 1;
					break;
				case KeyRelease:
					key = XLookupKeysym(&event.xkey,0);
					switch(key) {
						case XK_Escape: keys[KEY_ESC] = 0; break;
						case XK_Return: keys[KEY_RETURN] = 0; break;
						case XK_Left: keys[KEY_LEFT] = 0; break;
						case XK_Right: keys[KEY_RIGHT] = 0; break;
						case XK_Up: keys[KEY_UP] = 0; break;
						case XK_Down: keys[KEY_DOWN] = 0; break;
					}
					if(key < 256) keys[key] = 0;
					break;
				case MotionNotify:
					mouseX = event.xmotion.x;
					mouseY = event.xmotion.y;
					break;
				case ButtonPress:
					mouseButton |= 1 << ((event.xbutton.button - 1));
					break;
				case ButtonRelease:
					if(event.xbutton.button < 4) mouseButton &= ~(1 << (event.xbutton.button - 1));
					break;
			}
		}
		
		if(counter++ == 10) {
			endTime = startTime;
			startTime = getTime(); 
			fps = counter * 1000.0 / (float)(startTime - endTime);
			counter = 0;
		}
		ifps = 1.0 / fps;
		
		idle();
		render();
		
		glXSwapBuffers(display,window);
		mouseButton &= ~(BUTTON_UP | BUTTON_DOWN);
	}
	
	glXDestroyContext(display,glxContext);
	XDestroyWindow(display,window);
	if(fullScreen) XF86VidModeSwitchToMode(display,screen,modes[0]);
	XCloseDisplay(display);
	
	return 0;
}

#else

/*	windows
 */
#include <windows.h>
#include <GL/gl.h>
#include "glapp.h"

#pragma comment (lib,"opengl32.lib")
#pragma comment (lib,"glu32.lib")
#pragma comment (lib,"winmm.lib")

extern int main(int argc,char **argv);

static HWND hwnd;
static HGLRC wglcontext;
static GLApp *glApp;

/*
 */
GLApp::GLApp() {
	memset(this,0,sizeof(GLApp));
}

/*
 */
LRESULT CALLBACK windowProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam) {
	switch(message) {
		case WM_SIZE:
			glApp->windowWidth = LOWORD(lparam);
			glApp->windowHeight = HIWORD(lparam);
			glViewport(0,0,glApp->windowWidth,glApp->windowHeight);
			break;
		case WM_KEYDOWN:
			switch(wparam) {
				case VK_ESCAPE: glApp->keys[GLApp::KEY_ESC] = 1; break;
				case VK_RETURN: glApp->keys[GLApp::KEY_RETURN] = 1; break;
				case VK_LEFT: glApp->keys[GLApp::KEY_LEFT] = 1; break;
				case VK_RIGHT: glApp->keys[GLApp::KEY_RIGHT] = 1; break;
				case VK_UP: glApp->keys[GLApp::KEY_UP] = 1; break;
				case VK_DOWN: glApp->keys[GLApp::KEY_DOWN] = 1; break;
			}
			if(wparam < 256) glApp->keys[tolower(wparam)] = 1;
			break;
		case WM_KEYUP:
			switch(wparam) {
				case VK_ESCAPE: glApp->keys[GLApp::KEY_ESC] = 0; break;
				case VK_RETURN: glApp->keys[GLApp::KEY_RETURN] = 0; break;
				case VK_LEFT: glApp->keys[GLApp::KEY_LEFT] = 0; break;
				case VK_RIGHT: glApp->keys[GLApp::KEY_RIGHT] = 0; break;
				case VK_UP: glApp->keys[GLApp::KEY_UP] = 0; break;
				case VK_DOWN: glApp->keys[GLApp::KEY_DOWN] = 0; break;
			}
			if(wparam < 256) glApp->keys[tolower(wparam)] = 0;
			break;
		case WM_DESTROY:
			glApp->done = 1;
			break;
		case WM_LBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_LEFT;
			break;
		case WM_LBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_LEFT;
			break;
		case WM_MBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_MIDDLE;
			break;
		case WM_MBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_MIDDLE;
			break;
		case WM_RBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_RIGHT;
			break;
		case WM_RBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_RIGHT;
			break;
		case 0x020A: //WM_MOUSEWHEEL:
			if((short)HIWORD(wparam) > 0) glApp->mouseButton |= GLApp::BUTTON_UP;
			else glApp->mouseButton |= GLApp::BUTTON_DOWN;
			break;
		case WM_MOUSEMOVE:
			POINT point;
			GetCursorPos(&point);
			glApp->mouseX = point.x;
			glApp->mouseY = point.y;
			break;
	}
	return DefWindowProc(hwnd,message,wparam,lparam);
}

/*
 */
int GLApp::setVideoMode(int w,int h,int fs) {

	if(done || hwnd) return 0;
	
	windowWidth = w;
	windowHeight = h;
	fullScreen = fs;
	
	glApp = this;
	
	HDC hdc;
	int pixelformat;
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	
	WNDCLASS wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)windowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLApp";
	if(!RegisterClass(&wc)) return -1;
	
	if(fullScreen) {
		DEVMODE settings;
		memset(&settings,0,sizeof(DEVMODE));
		settings.dmSize = sizeof(DEVMODE);
		settings.dmPelsWidth = w;
		settings.dmPelsHeight = h;
		settings.dmBitsPerPel = 32;
		settings.dmDisplayFrequency = 85;
		settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
		if(ChangeDisplaySettings(&settings,CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) return 0;
		hwnd = CreateWindowEx(0,"GLApp","GLApp",WS_POPUP,0,0,windowWidth,windowHeight,NULL,NULL,hInstance,NULL);
	} else {
		hwnd = CreateWindowEx(0,"GLApp","GLApp",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,NULL,NULL,hInstance,NULL);
	}
	
	if(!hwnd) return -1;
	
	if(!(hdc = GetDC(hwnd))) return -1;
	if(!(pixelformat = ChoosePixelFormat(hdc,&pfd))) return -1;
	if(!(SetPixelFormat(hdc,pixelformat,&pfd))) return -1;
	if(!(wglcontext = wglCreateContext(hdc))) return -1;
	if(!wglMakeCurrent(hdc,wglcontext)) return -1;
	ShowWindow(hwnd,SW_SHOW);
	UpdateWindow(hwnd);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);
	
	return 0;
}

/*
 */
int GLApp::setTitle(const char *title) {
	SetWindowText(hwnd,title);
	return 0;
}

/*
 */
int GLApp::setPointer(int x,int y) {
	SetCursorPos(x,y);
	return 0;
}

/*
 */
int GLApp::showPointer(int show) {
	ShowCursor(show);
	return 0;
}

/*
 */
static int getTime() {
	static int base;
	static int initialized = 0;
	if(!initialized) {
		base = timeGetTime();
		initialized = 1;
	}
	return timeGetTime() - base;
}

/*
 */
int GLApp::main() {
	int startTime = 0,endTime = 0,counter = 0;
	fps = 60;
	MSG msg;
	while(!done) {
		if(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
			if(!GetMessage(&msg,NULL,0,0)) return msg.wParam;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			if(counter++ == 10) {
				endTime = startTime;
				startTime = getTime();
				fps = counter * 1000.0f / (float)(startTime - endTime);
				counter = 0;
			}
			ifps = 1.0f / fps;
			
			idle();
			render();
			
			SwapBuffers(wglGetCurrentDC());
			mouseButton &= ~(BUTTON_UP | BUTTON_DOWN);
		}
	}
	return msg.wParam;
}

/*
 */
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {
	int argc = 1;
	char *argv[256];
	argv[0] = "none";
	while(*lpCmdLine && argc < 256) {
		while(*lpCmdLine && *lpCmdLine <= ' ') lpCmdLine++;
		if(*lpCmdLine) {
			argv[argc++] = lpCmdLine;
			while(*lpCmdLine && *lpCmdLine > ' ') lpCmdLine++;
			if(*lpCmdLine) *(lpCmdLine++) = 0;
		}
	}
	main(argc,argv);
	if(glApp && glApp->fullScreen) ChangeDisplaySettings(NULL,0);
	return 0;
}

#endif
