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

#ifndef _WIN32

/* linux
 */
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif
#include "glapp.h"

static Display *display = NULL;
static int screen = 0;
static int num_modes = 0;
static XF86VidModeModeInfo **modes = NULL;
static Window window = 0;
static GLXContext context = 0;
static Atom WM_DELETE_WINDOW;
static Cursor cursor;

/*
 */
GLApp::GLApp() {
	
	done = 0;
	memset(keys,0,sizeof(keys));
	windowWidth = 0;
	windowHeight = 0;
	flags = 0;
	strcpy(title,"GLApp http://frustum.org");
	mouseX = 0;
	mouseY = 0;
	mouseButton = 0;
	fps = 60.0f;
	ifps = 1.0f / fps;
	
#ifdef HAVE_GTK
	gtk_disable_setlocale();
	gtk_init(NULL,NULL);
	display = GDK_DISPLAY();
#else
	display = XOpenDisplay(NULL);
	if(!display) exit("couldn`t open display");
#endif
	
}

GLApp::~GLApp() {
	if(context) glXDestroyContext(display,context);
	if(window) XDestroyWindow(display,window);
	if(modes) XF86VidModeSwitchToMode(display,screen,modes[0]);
	
#ifndef HAVE_GTK
	if(display) XCloseDisplay(display);
#endif
	
}

/*****************************************************************************/
/*                                                                           */
/* set video mode                                                            */
/*                                                                           */
/*****************************************************************************/

/*
 */
static int mode_cmp(const void *pa,const void *pb) {
	XF86VidModeModeInfo *a = *(XF86VidModeModeInfo**)pa;
	XF86VidModeModeInfo *b = *(XF86VidModeModeInfo**)pb;
	if(a->hdisplay > b->hdisplay) return -1;
	return b->vdisplay - a->vdisplay;
}

/*
 */
int GLApp::setVideoMode(int w,int h,int flags) {
	
	if(window) {
		XDestroyWindow(display,window);
		if(modes) XF86VidModeSwitchToMode(display,screen,modes[0]);
	}
	
	windowWidth = w;
	windowHeight = h;
	this->flags = flags;
	
	std::vector<int> attrib;
	attrib.push_back(GLX_RGBA);
	attrib.push_back(GLX_DOUBLEBUFFER);
	attrib.push_back(GLX_RED_SIZE);
	attrib.push_back(8);
	attrib.push_back(GLX_GREEN_SIZE);
	attrib.push_back(8);
	attrib.push_back(GLX_BLUE_SIZE);
	attrib.push_back(8);
	attrib.push_back(GLX_ALPHA_SIZE);
	attrib.push_back(8);
	attrib.push_back(GLX_DEPTH_SIZE);
	attrib.push_back(24);
	attrib.push_back(GLX_STENCIL_SIZE);
	attrib.push_back(8);
	if(flags & MULTISAMPLE_2 || flags & MULTISAMPLE_4) {
		attrib.push_back(GLX_SAMPLE_BUFFERS_ARB);
		attrib.push_back(true);
		attrib.push_back(GLX_SAMPLES_ARB);
		attrib.push_back(flags & MULTISAMPLE_2 ? 2 : 4);
	}
	attrib.push_back(None);
	
	try {
		
		screen = DefaultScreen(display);
		Window rootWindow = RootWindow(display,screen);
		
		XVisualInfo *visual = glXChooseVisual(display,screen,&attrib[0]);
		if(!visual) throw("can`t get rgba double-buffered visual");
		
		if(flags & FULLSCREEN) {
			if(!modes) {
				if(!XF86VidModeGetAllModeLines(display,screen,&num_modes,&modes)) throw("can`t get mode lines");
				qsort(modes,num_modes,sizeof(XF86VidModeModeInfo*),mode_cmp);
			}
			for(int i = 0; i < num_modes; i++) {
				if(modes[i]->hdisplay <= windowWidth && modes[i]->vdisplay <= windowHeight) {
					windowWidth = modes[i]->hdisplay;
					windowHeight = modes[i]->vdisplay;
					XF86VidModeSwitchToMode(display,screen,modes[i]);
					XF86VidModeSetViewPort(display,screen,0,0);
					break;
				}
			}
		}
		
		XSetWindowAttributes attr;
		unsigned long mask;
		attr.background_pixel = 0;
		attr.border_pixel = 0;
		attr.colormap = XCreateColormap(display,rootWindow,visual->visual,AllocNone);
		attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
		if(flags & FULLSCREEN) {
			mask = CWBackPixel | CWColormap | CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWEventMask;
			attr.override_redirect = True;
			attr.backing_store = NotUseful;
			attr.save_under = False;
		} else {
			mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
		}
		
		window = XCreateWindow(display,rootWindow,0,0,windowWidth,windowHeight,0,visual->depth,InputOutput,visual->visual,mask,&attr);
		XMapWindow(display,window);
		
		if(flags & FULLSCREEN) {
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
		
		if(!context) context = glXCreateContext(display,visual,NULL,True);
		if(!context) throw("glXCreateContext failed");
		
		glXMakeCurrent(display,window,context);
		glViewport(0,0,windowWidth,windowHeight);
		
		XStoreName(display,window,title);
		XSetIconName(display,window,title);
	}
	
	catch(const char *error) {
		window = 0;
		exit(error);
		return 0;
	}
	
	return 1;
}

/*
 */
void GLApp::setTitle(const char *title) {
	strcpy(this->title,title);
	XStoreName(display,window,title);
	XSetIconName(display,window,title);
	XFlush(display);
}

/*
 */
void GLApp::setCursor(int x,int y) {
	XWarpPointer(display,None,window,0,0,0,0,x,y);
	XFlush(display);
}

/*
 */
void GLApp::showCursor(int show) {
	if(show) XDefineCursor(display,window,0);
	else {
		if(!cursor) {
			char data[] = { 0 };
			Pixmap blank;
			XColor dummy;
			blank = XCreateBitmapFromData(display,window,data,1,1);
			cursor = XCreatePixmapCursor(display,blank,blank,&dummy,&dummy,0,0);
			XFreePixmap(display,blank);
		}
		XDefineCursor(display,window,cursor);
	}
}

/*
 */
int GLApp::checkExtension(const char *extension) {
	static char *extensions = NULL;
	if(!extensions) extensions = (char*)glGetString(GL_EXTENSIONS);
	if(strstr(extensions,extension)) return 1;
	char error[1024];
	sprintf(error,"OpenGL extension \"%s\" is not supported by current hardware",extension);
	exit(error);
	return 0;
}

/*
 */
void GLApp::error() {
	static int count = 0;
	static GLenum old_error = GL_NO_ERROR;
	while(1) {
		GLenum error = glGetError();
		if(error == GL_NO_ERROR) break;
		if(error == old_error) {
			fprintf(stderr,"OpenGL error 0x%04X: %s x %d\r",error,gluErrorString(error),count);
			count++;
		} else {
#ifdef HAVE_GTK
			GtkWidget *dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"OpenGL error 0x%04X: %s",error,(char*)gluErrorString(error));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
#endif
			fprintf(stderr,"%sOpenGL error 0x%04X: %s\r",old_error != GL_NO_ERROR ? "\n" : "",error,gluErrorString(error));
			count = 0;
			old_error = error;
		}
	}
}

/*
 */
void GLApp::exit(const char *error,...) {
	if(error) {
		char buf[1024];
		va_list arg;
		va_start(arg,error);
		vsprintf(buf,error,arg);
		va_end(arg);
#ifdef HAVE_GTK
		GtkWidget *dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"GLApp exit: %s\n",buf);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
#else
		fprintf(stderr,"GLApp exit: %s\n",buf);
#endif
	}
	done = 1;
}

/*
 */
#ifdef HAVE_GTK

static int ret;
static char *name;

static void signal_ok(GtkWidget *widget) {
	ret = 1;
	strcpy(name,gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget)));
	gtk_widget_destroy(widget);
	gtk_main_quit();
}

static void signal_cancel(GtkWidget *widget) {
	gtk_widget_destroy(widget);
	gtk_main_quit();
}

int GLApp::selectFile(const char *title,char *name) {
	ret = 0;
	::name = name;
	GtkWidget *window = gtk_file_selection_new(title);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(signal_cancel),&window);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),"clicked",GTK_SIGNAL_FUNC(signal_ok),GTK_OBJECT(window));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),"clicked",GTK_SIGNAL_FUNC(signal_cancel),GTK_OBJECT(window));
	gtk_widget_show(window);
	gtk_main();
	return ret;
}

#else

int GLApp::selectFile(const char *title,char *name) {
	printf("%s\n",title);
	fscanf(stdin,"%s",name);
	return 1;
}

#endif	/* HAVE_GTK */

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

static int translateKey(int key) {
	int ret;
	if(key & 0xff00) {
		switch(key) {
			case XK_Escape: ret = GLApp::KEY_ESC; break;
			case XK_Tab: ret = GLApp::KEY_TAB; break;
			case XK_Return: ret = GLApp::KEY_RETURN; break;
			case XK_BackSpace: ret = GLApp::KEY_BACKSPACE; break;
			case XK_Delete: ret = GLApp::KEY_DELETE; break;
			case XK_Home: ret = GLApp::KEY_HOME; break;
			case XK_End: ret = GLApp::KEY_END; break;
			case XK_Page_Up: ret = GLApp::KEY_PGUP; break;
			case XK_Page_Down: ret = GLApp::KEY_PGDOWN; break;
			case XK_Left: ret = GLApp::KEY_LEFT; break;
			case XK_Right: ret = GLApp::KEY_RIGHT; break;
			case XK_Up: ret = GLApp::KEY_UP; break;
			case XK_Down: ret = GLApp::KEY_DOWN; break;
			case XK_Shift_L: case XK_Shift_R: ret = GLApp::KEY_SHIFT; break;
			case XK_Control_L: case XK_Control_R: ret = GLApp::KEY_CTRL; break;
			case XK_Alt_L: case XK_Alt_R: ret = GLApp::KEY_ALT; break;
			case XK_F1: ret = GLApp::KEY_F1; break;
			case XK_F2: ret = GLApp::KEY_F2; break;
			case XK_F3: ret = GLApp::KEY_F3; break;
			case XK_F4: ret = GLApp::KEY_F4; break;
			case XK_F5: ret = GLApp::KEY_F5; break;
			case XK_F6: ret = GLApp::KEY_F6; break;
			case XK_F7: ret = GLApp::KEY_F7; break;
			case XK_F8: ret = GLApp::KEY_F8; break;
			case XK_F9: ret = GLApp::KEY_F9; break;
			case XK_F10: ret = GLApp::KEY_F10; break;
			case XK_F11: ret = GLApp::KEY_F11; break;
			case XK_F12: ret = GLApp::KEY_F12; break;
			default: ret = (key < 0x06ff) ? (key & 0x00ff) : 0;
		}
	} else {
		ret = key;
	}
	return ret;
}

/*
 */
void GLApp::update() {
	static int startTime = getTime();
	static int endTime = 0;
	static int counter = 0;
	
	while(XPending(display) > 0) {
		KeySym key;
		XEvent event;
		XNextEvent(display,&event);
		switch(event.type) {
			case ClientMessage:
				if(event.xclient.format == 32 && event.xclient.data.l[0] == (long)WM_DELETE_WINDOW) exit();
				break;
			case ConfigureNotify:
				windowWidth = event.xconfigure.width;
				windowHeight = event.xconfigure.height;
				glViewport(0,0,windowWidth,windowHeight);
				break;
			case KeyPress:
				XLookupString(&event.xkey,NULL,0,&key,NULL);
				key = translateKey(key);
				keys[key] = 1;
				if(keys[KEY_ALT] && keys[KEY_RETURN]) {
					keys[KEY_ALT] = 0;
					keys[KEY_RETURN] = 0;
					flags ^= FULLSCREEN;
					setVideoMode(windowWidth,windowHeight,flags);
				}
				if(key) keyPress(key);
				break;
			case KeyRelease:
				XLookupString(&event.xkey,NULL,0,&key,NULL);
				key = translateKey(key);
				keys[key] = 0;
				if(key) keyRelease(key);
				break;
			case MotionNotify:
				mouseX = event.xmotion.x;
				mouseY = event.xmotion.y;
				break;
			case ButtonPress:
				mouseButton |= 1 << ((event.xbutton.button - 1));
				buttonPress(1 << (event.xbutton.button - 1));
				break;
			case ButtonRelease:
				if(event.xbutton.button < 4) mouseButton &= ~(1 << (event.xbutton.button - 1));
				buttonRelease(1 << (event.xbutton.button - 1));
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
}

/*
 */
void GLApp::swap() {
	glXSwapBuffers(display,window);
	mouseButton &= ~(BUTTON_UP | BUTTON_DOWN);
}

/*
 */
void GLApp::main() {
	while(window && !done) {
		update();
		
		idle();		// call virtual functions
		render();
		
		swap();
	}
}

#else

/*****************************************************************************/
/*                                                                           */
/* windows                                                                   */
/*                                                                           */
/*****************************************************************************/

/*
 */
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglext.h>
#include "glapp.h"

#pragma comment (lib,"opengl32.lib")
#pragma comment (lib,"glu32.lib")
#pragma comment (lib,"winmm.lib")

extern int main(int argc,char **argv);

static GLApp *glApp = NULL;
static HWND window = 0;
static HDC hdc;
static int fullscreen = 0;
static HGLRC context = 0;
static HCURSOR cursor = 0;
static HCURSOR arrow_cursor = 0;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;

/*
 */
GLApp::GLApp() {
	done = 0;
	memset(keys,0,sizeof(keys));
	windowWidth = 0;
	windowHeight = 0;
	flags = 0;
	strcpy(title,"GLApp http://frustum.org");
	mouseX = 0;
	mouseY = 0;
	mouseButton = 0;
	fps = 60.0f;
	ifps = 1.0f / fps;
}

GLApp::~GLApp() {
	if(window) {
		wglMakeCurrent(hdc,0);
		wglDeleteContext(context);
		ReleaseDC(window,hdc);
		DestroyWindow(window);
		if(fullscreen) {
			ChangeDisplaySettings(NULL,0);
			ShowCursor(true);
		}
	}
}

/*
 */
static int translateKey(int key) {
	int ret;
	switch(key) {
		case VK_ESCAPE : ret = GLApp::KEY_ESC; break;
		case VK_TAB: ret = GLApp::KEY_TAB; break;
		case VK_RETURN: ret = GLApp::KEY_RETURN; break;
		case VK_BACK: ret = GLApp::KEY_BACKSPACE; break;
		case VK_DELETE: ret = GLApp::KEY_DELETE; break;
		case VK_HOME: ret = GLApp::KEY_HOME; break;
		case VK_END: ret = GLApp::KEY_END; break;
		case VK_PRIOR: ret = GLApp::KEY_PGUP; break;
		case VK_NEXT: ret = GLApp::KEY_PGDOWN; break;
		case VK_LEFT: ret = GLApp::KEY_LEFT; break;
		case VK_RIGHT: ret = GLApp::KEY_RIGHT; break;
		case VK_UP: ret = GLApp::KEY_UP; break;
		case VK_DOWN: ret = GLApp::KEY_DOWN; break;
		case VK_SHIFT: ret = GLApp::KEY_SHIFT; break;
		case VK_MENU: ret = GLApp::KEY_ALT; break;
		case VK_CONTROL: ret = GLApp::KEY_CTRL; break;
		default:
			ret = MapVirtualKey(key,2);
			if(strchr("1234567890-=",ret)) {
				if(glApp->keys[GLApp::KEY_SHIFT]) {
					if(ret == '1') ret = '!';
					else if(ret == '2') ret = '@';
					else if(ret == '3') ret = '#';
					else if(ret == '4') ret = '$';
					else if(ret == '5') ret = '%';
					else if(ret == '6') ret = '^';
					else if(ret == '7') ret = '&';
					else if(ret == '8') ret = '*';
					else if(ret == '9') ret = '(';
					else if(ret == '0') ret = ')';
					else if(ret == '-') ret = '_';
					else if(ret == '=') ret = '+';
				}
			} else if(isascii(ret)) {
				if(glApp->keys[GLApp::KEY_SHIFT]) ret = toupper(ret);
				else ret = tolower(ret);
			}
			else ret = 0;
	}
	return ret;
}

/*
 */
LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
	switch(message) {
		case WM_SIZE:
			glApp->windowWidth = LOWORD(lparam);
			glApp->windowHeight = HIWORD(lparam);
			glViewport(0,0,glApp->windowWidth,glApp->windowHeight);
			return 1;
		case WM_SETCURSOR:
			SetCursor(cursor);
			return 1;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
				int key = translateKey((int)wparam);
				glApp->keys[key] = 1;
				if(glApp->keys[GLApp::KEY_ALT] && glApp->keys[GLApp::KEY_RETURN]) {
					glApp->keys[GLApp::KEY_ALT] = 0;
					glApp->keys[GLApp::KEY_RETURN] = 0;
					glApp->flags ^= GLApp::FULLSCREEN;
					glApp->setVideoMode(glApp->windowWidth,glApp->windowHeight,glApp->flags);
					glApp->setTitle(glApp->title);
				}
				glApp->keyPress(key);
			}
			return 1;
		case WM_SYSKEYUP:
		case WM_KEYUP: {
				int key = translateKey((int)wparam);
				glApp->keys[key] = 0;
				glApp->keyRelease(key);
			}
			return 1;
		case WM_CLOSE:
			glApp->exit();
			return 1;
		case WM_LBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_LEFT;
			glApp->buttonPress(GLApp::BUTTON_LEFT);
			return 1;
		case WM_LBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_LEFT;
			glApp->buttonRelease(GLApp::BUTTON_LEFT);
			return 1;
		case WM_MBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_MIDDLE;
			glApp->buttonPress(GLApp::BUTTON_MIDDLE);
			return 1;
		case WM_MBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_MIDDLE;
			glApp->buttonRelease(GLApp::BUTTON_MIDDLE);
			return 1;
		case WM_RBUTTONDOWN:
			glApp->mouseButton |= GLApp::BUTTON_RIGHT;
			glApp->buttonPress(GLApp::BUTTON_RIGHT);
			return 1;
		case WM_RBUTTONUP:
			glApp->mouseButton &= ~GLApp::BUTTON_RIGHT;
			glApp->buttonRelease(GLApp::BUTTON_RIGHT);
			return 1;
		case 0x020A: //WM_MOUSEWHEEL:
			if((short)HIWORD(wparam) > 0) {
				glApp->mouseButton |= GLApp::BUTTON_UP;
				glApp->buttonPress(GLApp::BUTTON_UP);
				glApp->buttonRelease(GLApp::BUTTON_UP);
			} else {
				glApp->mouseButton |= GLApp::BUTTON_DOWN;
				glApp->buttonPress(GLApp::BUTTON_DOWN);
				glApp->buttonRelease(GLApp::BUTTON_DOWN);
			}
			return 1;
		case WM_MOUSEMOVE:
			glApp->mouseX = LOWORD(lparam);
			glApp->mouseY = HIWORD(lparam);
			if(glApp->mouseX & 1 << 15) glApp->mouseX -= (1 << 16);
			if(glApp->mouseY & 1 << 15) glApp->mouseY -= (1 << 16);
			return 1;
	}
	return DefWindowProc(window,message,wparam,lparam);
}

/*
 */
int GLApp::setVideoMode(int w,int h,int flags) {
	
	if(window) {
		ReleaseDC(window,hdc);
		DestroyWindow(window);
		if(fullscreen) {
			ChangeDisplaySettings(NULL,0);
			ShowCursor(true);
		}
	}
	
	windowWidth = w;
	windowHeight = h;
	this->flags = flags;

	glApp = this;
	fullscreen = flags & FULLSCREEN;
	
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	
	std::vector<int> attrib;
	attrib.push_back(WGL_DRAW_TO_WINDOW_ARB);
	attrib.push_back(true);
	attrib.push_back(WGL_SUPPORT_OPENGL_ARB);
	attrib.push_back(true);
	attrib.push_back(WGL_ACCELERATION_ARB);
	attrib.push_back(WGL_FULL_ACCELERATION_ARB);
	attrib.push_back(WGL_DOUBLE_BUFFER_ARB);
	attrib.push_back(true);
	attrib.push_back(WGL_RED_BITS_ARB);
	attrib.push_back(8);
	attrib.push_back(WGL_GREEN_BITS_ARB);
	attrib.push_back(8);
	attrib.push_back(WGL_BLUE_BITS_ARB);
	attrib.push_back(8);
	attrib.push_back(WGL_ALPHA_BITS_ARB);
	attrib.push_back(8);
	attrib.push_back(WGL_DEPTH_BITS_ARB);
	attrib.push_back(24);
	attrib.push_back(WGL_STENCIL_BITS_ARB);
	attrib.push_back(8);
	if(flags & MULTISAMPLE_2 || flags & MULTISAMPLE_4) {
		attrib.push_back(WGL_SAMPLE_BUFFERS_ARB);
		attrib.push_back(true);
		attrib.push_back(WGL_SAMPLES_ARB);
		attrib.push_back(flags & MULTISAMPLE_2 ? 2 : 4);
	}
	attrib.push_back(0);
	
	try {
		
		HINSTANCE hInstance = GetModuleHandle(NULL);
		if(!window) {
			WNDCLASS wc;
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
			if(!RegisterClass(&wc)) throw("RegisterClass() failed");
		}

		if(flags & FULLSCREEN) {
			
			static int widths[] = { 320, 400, 640, 800, 1024, 1280, 1600 };
			static int heights[] = { 240, 300, 480, 600, 768, 960, 1200 };
			for(int i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
				if(widths[i] >= windowWidth) {
					windowWidth = widths[i];
					windowHeight = heights[i];
					break;
				}
			}
			
			DEVMODE settings;
			memset(&settings,0,sizeof(DEVMODE));
			settings.dmSize = sizeof(DEVMODE);
			settings.dmPelsWidth = windowWidth;
			settings.dmPelsHeight = windowHeight;
			settings.dmBitsPerPel = 32;
			settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
			
			if(ChangeDisplaySettings(&settings,CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) throw("ChangeDisplaySettings() failed");
			
			window = CreateWindowEx(WS_EX_TOPMOST,"GLApp","GLApp",WS_POPUP | WS_VISIBLE,0,0,windowWidth,windowHeight,NULL,NULL,hInstance,NULL);
			
		} else {
			
			RECT windowRect = {0, 0, windowWidth, windowHeight };
			AdjustWindowRectEx(&windowRect,WS_OVERLAPPEDWINDOW,0,0);
			
			windowWidth = windowRect.right - windowRect.left;
			windowHeight = windowRect.bottom - windowRect.top;
			
			window = CreateWindowEx(0,"GLApp","GLApp",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,NULL,NULL,hInstance,NULL);
		}
		
		if(!window) throw("CreateWindowEx() failed");
		
		hdc = GetDC(window);
		if(!hdc) throw("GetDC() failed");
		
		// create simple ogl context
		if(!wglChoosePixelFormatARB) {
			int pixelformat = ChoosePixelFormat(hdc,&pfd);
			SetPixelFormat(hdc,pixelformat,&pfd);
			
			// create context
			if(!context) context = wglCreateContext(hdc);
			if(!context) throw("wglCreateContext() failed");
			if(!wglMakeCurrent(hdc,context)) throw("wglMakeCurrent() failed");
			
			// get proc address
			wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			
			if(!wglChoosePixelFormatARB) {
				
				glViewport(0,0,glApp->windowWidth,glApp->windowHeight);
				
				ShowWindow(window,SW_SHOW);
				UpdateWindow(window);
				SetForegroundWindow(window);
				SetFocus(window);
				
				SetWindowText(window,title);
				
				arrow_cursor = LoadCursor(NULL,IDC_ARROW);
				cursor = arrow_cursor;
				
				return 1;
			}
			
			// destroy context
			wglMakeCurrent(hdc,0);
			wglDeleteContext(context);
			
			context = 0;
			
			// set video mode again
			return setVideoMode(w,h,flags);
		}
		
		int pixelformat;
		unsigned int count;
		wglChoosePixelFormatARB(hdc,&attrib[0],NULL,1,&pixelformat,&count);
		if(count == 0) throw("wglChoosePixelFormatARB() failed");
		
		SetPixelFormat(hdc,pixelformat,&pfd);
		
		if(!context) context = wglCreateContext(hdc);
		if(!context) throw("wglCreateContext() failed");
		
		if(!wglMakeCurrent(hdc,context)) throw("wglMakeCurrent() failed");
		
		glViewport(0,0,glApp->windowWidth,glApp->windowHeight);
		if(flags & MULTISAMPLE_2 || flags & MULTISAMPLE_4) glEnable(GL_MULTISAMPLE_ARB);
		
		ShowWindow(window,SW_SHOW);
		UpdateWindow(window);
		SetForegroundWindow(window);
		SetFocus(window);
		
		SetWindowText(window,title);
		
		arrow_cursor = LoadCursor(NULL,IDC_ARROW);
		cursor = arrow_cursor;
	}
	
	catch(const char *error) {
		window = 0;
		exit(error);
		return 0;
	}
	
	return 1;
}

/*
 */
void GLApp::setTitle(const char *title) {
	strcpy(this->title,title);
	SetWindowText(window,title);
}

/*
 */
void GLApp::setCursor(int x,int y) {
	POINT pt;
	pt.x = x;
	pt.y = y;
	ClientToScreen(window,&pt);
	SetCursorPos(pt.x,pt.y);
}

/*
 */
void GLApp::showCursor(int show) {
	if(show) cursor = arrow_cursor;
	else cursor = NULL;
	SetCursor(cursor);
}

/*
 */
int GLApp::checkExtension(const char *extension) {
	static char *extensions = NULL;
	if(!extensions) extensions = (char*)glGetString(GL_EXTENSIONS);
	if(strstr(extensions,extension)) return 1;
	char error[1024];
	sprintf(error,"OpenGL extension \"%s\" is not supported by current hardware",extension);
	exit(error);
	return 0;
}

/*
 */
void GLApp::error() {
	static int count = 0;
	static GLenum old_error = GL_NO_ERROR;
	while(1) {
		GLenum error = glGetError();
		if(error == GL_NO_ERROR) break;
		if(error == old_error) {
			fprintf(stderr,"OpenGL error 0x%04X: %s x %d\n",error,gluErrorString(error),count);
			count++;
		} else {
			MessageBox(0,(char*)gluErrorString(error),"OpenGL error",MB_OK);
			fprintf(stderr,"OpenGL error 0x%04X: %s\n",error,gluErrorString(error));
			count = 0;
			old_error = error;
		}
	}
}

/*
 */
void GLApp::exit(const char *error,...) {
	if(error) {
		char buf[1024];
		va_list arg;
		va_start(arg,error);
		vsprintf(buf,error,arg);
		va_end(arg);
		MessageBox(0,buf,"GLApp exit",MB_OK);
	}
	done = 1;
}

/*
 */
int GLApp::selectFile(const char *title,char *name) {
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = window;
	ofn.lpstrFileTitle = name;
	ofn.nMaxFileTitle = 512;
	ofn.lpstrFilter = "*.*";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_FILEMUSTEXIST;
	return GetOpenFileName(&ofn);
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
void GLApp::update() {
	static int startTime = getTime();
	static int endTime = 0;
	static int counter = 0;
	
	MSG msg;
	while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
		if(!GetMessage(&msg,NULL,0,0)) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	if(counter++ == 10) {
		endTime = startTime;
		startTime = getTime(); 
		fps = counter * 1000.0f / (float)(startTime - endTime);
		counter = 0;
	}
	ifps = 1.0f / fps;
}

/*
 */
void GLApp::swap() {
	SwapBuffers(wglGetCurrentDC());
	mouseButton &= ~(BUTTON_UP | BUTTON_DOWN);
}

/*
 */
void GLApp::main() {
	while(window && !done) {
		update();
		
		idle();		// call virtual functions
		render();
		
		swap();
	}
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
	
	FILE *file = fopen("stderr.txt","w");
	*stderr = *file;
	setbuf(stderr,NULL);

	main(argc,argv);
	
	fclose(file);
	file = fopen("stderr.txt","r");
	char eof = fgetc(file);
	fclose(file);
	if(eof == EOF) remove("stderr.txt");
	
	return 0;
}

#endif
