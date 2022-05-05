/* OpenGL App
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

#ifndef _WIN32

/* linux
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
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

#include "GLApp.h"

// GLApp lock
static int init = 0;
static GLApp *glApp = NULL;

// X11 Window
static Display *display = NULL;
static int screen = 0;
static XF86VidModeGamma gamma;
static int num_modes = 0;
static XF86VidModeModeInfo **modes = NULL;
static Window window = 0;
static GLXContext context = 0;
static Atom WM_DELETE_WINDOW;
static int show_mouse = 1;
static Cursor cursor = 0;

// release buffers
static int num_keys = 0;
static int keys_buffer[64];
static int button_mask = 0;

// fps counter
static int fps_init = 0;
static int fps_counter = 0;
static long long fps_time = 0;
static long long fps_delta = 0;

/*
 */
static void signal_handler(int signal) {
	if(glApp) glApp->exit("received signal %d\n",signal);
	else fprintf(stderr,"received signal %d\n",signal);
}

/*
 */
GLApp::GLApp() {
	
	if(init) {
		fprintf(stderr,"GLApp::GLApp(): already created\n");
		::exit(1);
	}
	glApp = this;
	
/*	signal(SIGINT,signal_handler);
	signal(SIGILL,signal_handler);
	signal(SIGFPE,signal_handler);
	signal(SIGSEGV,signal_handler);
	signal(SIGTERM,signal_handler);
*/	
	done = 0;
	
	gl_info = NULL;
	gl_version = NULL;
	gl_extensions = NULL;
	wglx_extensions = NULL;
	
	window_width = 0;
	window_height = 0;
	flags = 0;
	strcpy(title,"GLApp http://frustum.org/");
	
	memset(keys,0,sizeof(keys));
	mouse_x = 0;
	mouse_y = 0;
	mouse_button = 0;
	
	fps = 60.0f;
	ifps = 1.0f / fps;
	
#ifdef HAVE_GTK
	gtk_disable_setlocale();
	gtk_init(NULL,NULL);
	display = GDK_DISPLAY();
#else
	display = XOpenDisplay(NULL);
#endif
	if(display == NULL) {
		exit("couldn`t open display");
		return;
	}
	
	XF86VidModeGetGamma(display,screen,&gamma);
}

GLApp::~GLApp() {
	
	init = 0;
	glApp = NULL;
	
	if(context) glXDestroyContext(display,context);
	if(window) XDestroyWindow(display,window);
	if(flags & FULLSCREEN) {
		if(modes) XF86VidModeSwitchToMode(display,screen,modes[0]);
	}
	if(display != NULL) {
		XF86VidModeSetGamma(display,screen,&gamma);
		XFlush(display);
	}
	
#ifndef HAVE_GTK
	if(display) XCloseDisplay(display);
#endif
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
void GLApp::setGamma(float value) {
	XF86VidModeGamma gamma;
	gamma.red = value;
	gamma.green = value;
	gamma.blue = value;
	XF86VidModeSetGamma(display,screen,&gamma);
}

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
int GLApp::setVideoMode(int width,int height,int flags) {
	
	if(display == NULL) return 0;
	
	if(window) {
		update();
		if((this->flags & FULLSCREEN) != (flags & FULLSCREEN)) {
			if(modes) XF86VidModeSwitchToMode(display,screen,modes[0]);
		}
		XFlush(display);
	}
	
	window_width = width;
	window_height = height;
	this->flags = flags;
	
	int attribs[128];
	memset(attribs,0,sizeof(attribs));
	int *aptr = attribs;
	
	*aptr++ = GLX_RGBA;
	*aptr++ = GLX_DOUBLEBUFFER;
	*aptr++ = GLX_RED_SIZE;
	*aptr++ = 8;
	*aptr++ = GLX_GREEN_SIZE;
	*aptr++ = 8;
	*aptr++ = GLX_BLUE_SIZE;
	*aptr++ = 8;
	*aptr++ = GLX_ALPHA_SIZE;
	*aptr++ = 8;
	*aptr++ = GLX_DEPTH_SIZE;
	*aptr++ = 24;
	*aptr++ = GLX_STENCIL_SIZE;
	*aptr++ = 8;
	if((flags & MULTISAMPLE_2) || (flags & MULTISAMPLE_4)) {
		*aptr++ = GLX_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = GLX_SAMPLES_ARB;
		*aptr++ = (flags & MULTISAMPLE_2) ? 2 : 4;
	}
	
	try {
		
		screen = DefaultScreen(display);
		Window root_window = RootWindow(display,screen);
		
		XVisualInfo *visual = glXChooseVisual(display,screen,attribs);
		if(visual == NULL) throw("couldn't get rgb double-buffered visual");
		
		if(flags & FULLSCREEN) {
			if(modes == NULL) {
				if(XF86VidModeGetAllModeLines(display,screen,&num_modes,&modes) == 0) throw("can`t get mode lines");
				qsort(modes,num_modes,sizeof(XF86VidModeModeInfo*),mode_cmp);
			}
			for(int i = 0; i < num_modes; i++) {
				if(modes[i]->hdisplay <= window_width && modes[i]->vdisplay <= window_height) {
					window_width = modes[i]->hdisplay;
					window_height = modes[i]->vdisplay;
					XF86VidModeSwitchToMode(display,screen,modes[i]);
					XF86VidModeSetViewPort(display,screen,0,0);
					break;
				}
			}
		}
		
		XSetWindowAttributes attr;
		unsigned long mask = 0;
		attr.background_pixel = 0;
		attr.border_pixel = 0;
		attr.colormap = XCreateColormap(display,root_window,visual->visual,AllocNone);
		attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | LeaveWindowMask | MapNotify | ButtonPressMask | ButtonReleaseMask | FocusChangeMask;
		if(flags & FULLSCREEN) {
			mask = CWBackPixel | CWColormap | CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWEventMask;
			attr.override_redirect = true;
			attr.backing_store = NotUseful;
			attr.save_under = false;
		} else {
			mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
		}
		
		Window old_window = window;
		
		window = XCreateWindow(display,root_window,0,0,window_width,window_height,0,visual->depth,InputOutput,visual->visual,mask,&attr);
		XMapWindow(display,window);
		
		if(flags & FULLSCREEN) {
			XMoveWindow(display,window,0,0);
			XRaiseWindow(display,window);
			XFlush(display);
			XF86VidModeSetViewPort(display,screen,0,0);
			XGrabPointer(display,window,true,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
			XGrabKeyboard(display,window,true,GrabModeAsync,GrabModeAsync,CurrentTime);
		} else {
			WM_DELETE_WINDOW = XInternAtom(display,"WM_DELETE_WINDOW",false);
			XSetWMProtocols(display,window,&WM_DELETE_WINDOW,1);
		}
		
		XFlush(display);
		
		Window root_ret;
		Window child_ret;
		int root_x_ret;
		int root_y_ret;
		unsigned int mask_ret;
		XQueryPointer(display,window,&root_ret,&child_ret,&root_x_ret,&root_y_ret,&mouse_x,&mouse_y,&mask_ret);
		
		context = glXCreateContext(display,visual,context,true);
		if(context == 0) throw("glXCreateContext() failed");
		
		if(glXMakeCurrent(display,window,context) == 0) throw("glXMakeCurrent() failed");
		glViewport(0,0,window_width,window_height);
		
		if(old_window) {
			XUnmapWindow(display,old_window);
			XDestroyWindow(display,old_window);
		}
		
		XFlush(display);
		
		XStoreName(display,window,title);
		XSetIconName(display,window,title);
		
		show_mouse = !show_mouse;
		showMouse(!show_mouse);
	}
	catch(const char *error) {
		if(modes) XF86VidModeSwitchToMode(display,screen,modes[0]);
		window = 0;
		exit(error);
		return 0;
	}
	
	// opengl info
	if(gl_info == NULL) gl_info = (const char*)glGetString(GL_RENDERER);
	if(gl_version == NULL) gl_version = (const char*)glGetString(GL_VERSION);
	if(gl_extensions == NULL) gl_extensions = (const char*)glGetString(GL_EXTENSIONS);
	if(wglx_extensions == NULL) wglx_extensions = (const char*)glXQueryExtensionsString(display,screen);
	
	return 1;
}

/*
 */
void GLApp::setTitle(const char *title) {
	if(window == 0) return;
	strcpy(this->title,title);
	XStoreName(display,window,title);
	XSetIconName(display,window,title);
	XFlush(display);
}

/*
 */
void GLApp::setMouse(int x,int y) {
	if(window == 0) return;
	mouse_x = x;
	mouse_y = y;
	XWarpPointer(display,None,window,0,0,0,0,x,y);
	XFlush(display);
}

/*
 */
void GLApp::showMouse(int show) {
	if(window == 0) return;
	if(show_mouse == show) return;
	show_mouse = show;
	if(show) {
		XDefineCursor(display,window,0);
		if(flags & FULLSCREEN) XGrabPointer(display,window,true,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
		else XUngrabPointer(display,CurrentTime);
	}
	else {
		if(cursor == 0) {
			char data[] = { 0 };
			Pixmap blank;
			XColor dummy;
			blank = XCreateBitmapFromData(display,window,data,1,1);
			cursor = XCreatePixmapCursor(display,blank,blank,&dummy,&dummy,0,0);
			XFreePixmap(display,blank);
		}
		XDefineCursor(display,window,cursor);
		XGrabPointer(display,window,true,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
	}
}

/*
 */
int GLApp::checkExtension(const char *extension) {
	if(gl_extensions == NULL) return 0;
	if(strstr(gl_extensions,extension)) return 1;
	return 0;
}

/*
 */
void GLApp::error() {
	while(1) {
		GLenum error = glGetError();
		if(error == GL_NO_ERROR) break;
		fprintf(stderr,"OpenGL error 0x%04X: %s\n",error,gluErrorString(error));
	}
}

/*
 */
void GLApp::exit(const char *error,...) {
	showMouse(1);
	if(error) {
		char buf[1024];
		va_list arg;
		va_start(arg,error);
		vsnprintf(buf,sizeof(buf),error,arg);
		va_end(arg);
#ifdef HAVE_GTK
		GtkWidget *dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"GLApp::exit(): %s\n",buf);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
#else
		fprintf(stderr,"GLApp::exit(): %s\n",buf);
#endif
	}
	done = 1;
}

/*
 */
#ifdef HAVE_GTK

static int ret = 0;
static char *name = NULL;

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
	if(name[0] != '\0') gtk_file_selection_set_filename(GTK_FILE_SELECTION(window),name);
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

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
static int translate_key(GLApp *glapp,XKeyEvent *event) {
	KeySym key;
	XLookupString(event,NULL,0,&key,NULL);
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
	if(ret >= GLApp::NUM_KEYS) ret = 0;
	return ret;
}

/*
 */
static long long get_time() {
	struct timeval tval;
	gettimeofday(&tval,NULL);
	return (long long)tval.tv_sec * 1000000 + tval.tv_usec;
}

/*
 */
void GLApp::stopFps() {
	fps_delta = get_time() - fps_time;
}

/*
 */
void GLApp::startFps() {
	fps_time = get_time() - fps_delta;
}

/*
 */
void GLApp::update() {
	
	if(window == 0) return;
	
	// release keys
	for(int i = 0; i < num_keys; i++) {
		if(keys_buffer[i] == 0) continue;
		keys[keys_buffer[i]] = 0;
	}
	num_keys = 0;
	
	// release mouse button
	mouse_button &= ~button_mask;
	button_mask = 0;
	
	// process events
	int unmap = 0;
	while(XPending(display) || unmap == 1) {
		int key;
		XEvent event;
		XNextEvent(display,&event);
		switch(event.type) {
			case ClientMessage:
				if(event.xclient.format == 32 && event.xclient.data.l[0] == (long)WM_DELETE_WINDOW) exit();
				break;
			case ConfigureNotify:
				window_width = event.xconfigure.width;
				window_height = event.xconfigure.height;
				glViewport(0,0,window_width,window_height);
				break;
			case KeyPress:
				key = translate_key(this,&event.xkey);
				if(key) keyPress(key);
				if(key >= 'A' && key <= 'Z') key -= 'A' - 'a';
				for(int i = 0; i < num_keys; i++) {
					if(keys_buffer[i] == key) keys_buffer[i] = 0;
				}
				keys[key] = 1;
				break;
			case KeyRelease:
				key = translate_key(this,&event.xkey);
				if(key) keyRelease(key);
				if(key >= 'A' && key <= 'Z') key -= 'A' - 'a';
				if(num_keys != sizeof(keys_buffer)) keys_buffer[num_keys++] = key;
				else keys[key] = 0;
				break;
			case MotionNotify:
				mouse_x = event.xmotion.x;
				mouse_y = event.xmotion.y;
				break;
			case LeaveNotify:
				break;
			case UnmapNotify:
				if(event.xunmap.window == window) {
					unmap = 1;
					stopFps();
				}
				break;
			case MapNotify:
				if(event.xmap.window == window) {
					unmap = 0;
					startFps();
					if(show_mouse == 0) XGrabPointer(display,window,true,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
				}
				break;
			case ButtonPress:
				mouse_button |= 1 << ((event.xbutton.button - 1));
				buttonPress(1 << (event.xbutton.button - 1));
				break;
			case ButtonRelease:
				if(event.xbutton.button < 4) button_mask |= 1 << (event.xbutton.button - 1);
				buttonRelease(1 << (event.xbutton.button - 1));
				break;
			case FocusIn:
			case FocusOut:
				memset(keys,0,sizeof(keys));
				mouse_button = 0;
				break;
		}
	}
	
	if(fps_init == 0) {
		fps_time = get_time();
		fps_counter = 0;
		fps_init = 1;
	}
	
	if(fps_counter++ == 10) {
		long long fps_old_time = fps_time;
		fps_time = get_time(); 
		fps = (float)((double)fps_counter * 1000000.0 / (double)(fps_time - fps_old_time));
		fps_counter = 0;
	}
	ifps = 1.0 / fps;
}

/*
 */
void GLApp::swap() {
	if(window == 0) return;
	glXSwapBuffers(display,window);
	mouse_button &= ~(BUTTON_UP | BUTTON_DOWN);
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
#define DIRECTINPUT_VERSION 0x0800

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <dinput.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglext.h>

#include "GLApp.h"

#pragma comment (lib,"opengl32.lib")
#pragma comment (lib,"glu32.lib")
#pragma comment (lib,"dinput8.lib")
#pragma comment (lib,"dxguid.lib")

// GLApp lock
static int init = 0;
static GLApp *glApp = NULL;

// window
static WORD gamma[3][256];
static HWND window = 0;
static HDC hdc;
static HGLRC context = 0;
static int show_mouse = 1;
static HCURSOR cursor = 0;
static HCURSOR arrow_cursor = 0;
static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
static int minimized = 0;

// release buffers
static int num_keys = 0;
static int keys_buffer[64];
static int button_mask = 0;

// direct input
LPDIRECTINPUT8 direct_input = NULL;
LPDIRECTINPUTDEVICE8 direct_mouse = NULL;

// fps counter
static int fps_init = 0;
static int fps_counter = 0;
static long long fps_time = 0;
static long long fps_delta = 0;

// function prototypes
static void free_direct_input();

/*
 */
GLApp::GLApp() {
	
	if(init) {
		fprintf(stderr,"GLApp::GLApp(): already created\n");
		::exit(1);
	}
	glApp = this;
	
	done = 0;
	
	gl_info = NULL;
	gl_version = NULL;
	gl_extensions = NULL;
	wglx_extensions = NULL;

	window_width = 0;
	window_height = 0;
	flags = 0;
	strcpy(title,"GLApp http://frustum.org/");
	
	memset(keys,0,sizeof(keys));
	mouse_x = 0;
	mouse_y = 0;
	mouse_button = 0;

	fps = 60.0f;
	ifps = 1.0f / fps;
	
	HDC hdc = GetDC(GetDesktopWindow());
	GetDeviceGammaRamp(hdc,gamma);
	ReleaseDC(GetDesktopWindow(),hdc);
}

GLApp::~GLApp() {
	
	init = 0;
	glApp = NULL;
	
	if(window) {
		free_direct_input();
		wglMakeCurrent(hdc,0);
		wglDeleteContext(context);
		ReleaseDC(window,hdc);
		DestroyWindow(window);
	}
	
	if(flags & FULLSCREEN) {
		ChangeDisplaySettings(NULL,0);
		ShowCursor(true);
	}

	HDC hdc = GetDC(GetDesktopWindow());
	SetDeviceGammaRamp(hdc,gamma);
	ReleaseDC(GetDesktopWindow(),hdc);
}

/*
 */
static int translate_key(int key) {
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
		case VK_F1: ret = GLApp::KEY_F1; break;
		case VK_F2: ret = GLApp::KEY_F2; break;
		case VK_F3: ret = GLApp::KEY_F3; break;
		case VK_F4: ret = GLApp::KEY_F4; break;
		case VK_F5: ret = GLApp::KEY_F5; break;
		case VK_F6: ret = GLApp::KEY_F6; break;
		case VK_F7: ret = GLApp::KEY_F7; break;
		case VK_F8: ret = GLApp::KEY_F8; break;
		case VK_F9: ret = GLApp::KEY_F9; break;
		case VK_F10: ret = GLApp::KEY_F10; break;
		case VK_F11: ret = GLApp::KEY_F11; break;
		case VK_F12: ret = GLApp::KEY_F12; break;
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
static void create_direct_input() {
	
	KillTimer(window,0);
	free_direct_input();
	
	if(FAILED(DirectInput8Create(GetModuleHandle(NULL),DIRECTINPUT_VERSION,IID_IDirectInput8,(void**)&direct_input,NULL))) throw("DirectInput8Create(): failed\n");
	if(FAILED(direct_input->CreateDevice(GUID_SysMouse,&direct_mouse,NULL))) throw("DirectInput8->CreateDevice(): failed\n");
	if(FAILED(direct_mouse->SetDataFormat(&c_dfDIMouse2))) throw("DirectInput8->SetDataFormat(): failed\n");
	direct_mouse->SetCooperativeLevel(window,DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	
	if(show_mouse == 0) direct_mouse->Acquire();
	
	SetTimer(window,0,1000 / 200,NULL);
}

/*
 */
static void free_direct_input() {
	
	if(direct_mouse) {
		direct_mouse->Unacquire();
		direct_mouse->Release();
	}
	if(direct_input) {
		direct_input->Release();
	}
	direct_input = NULL;
	direct_mouse = NULL;
}

/*
 */
static void read_direct_input() {
	
	if(show_mouse) return;
	if(direct_mouse == NULL) return;

	DIMOUSESTATE2 state;
	if(FAILED(direct_mouse->GetDeviceState(sizeof(DIMOUSESTATE2),&state))) return;
	
	glApp->mouse_x += state.lX;
	glApp->mouse_y += state.lY;
	
	if(glApp->mouse_x < 0) glApp->mouse_x = 0;
	else if(glApp->mouse_x >= glApp->window_width) glApp->mouse_x = glApp->window_width - 1;
	if(glApp->mouse_y < 0) glApp->mouse_y = 0;
	else if(glApp->mouse_y >= glApp->window_height) glApp->mouse_y = glApp->window_height - 1;
	
	int old_mouse_button = glApp->mouse_button;
	glApp->mouse_button = 0;
	
	if(old_mouse_button & GLApp::BUTTON_LEFT) {
		if(state.rgbButtons[0] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_LEFT;
		} else {
			button_mask |= GLApp::BUTTON_LEFT;
			glApp->buttonRelease(GLApp::BUTTON_LEFT);
		}
	} else {
		if(state.rgbButtons[0] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_LEFT;
			glApp->buttonPress(GLApp::BUTTON_LEFT);
		}
	}
	
	if(old_mouse_button & GLApp::BUTTON_MIDDLE) {
		if(state.rgbButtons[2] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_MIDDLE;
		} else {
			button_mask |= GLApp::BUTTON_MIDDLE;
			glApp->buttonRelease(GLApp::BUTTON_MIDDLE);
		}
	} else {
		if(state.rgbButtons[2] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_MIDDLE;
			glApp->buttonPress(GLApp::BUTTON_MIDDLE);
		}
	}
	
	if(old_mouse_button & GLApp::BUTTON_RIGHT) {
		if(state.rgbButtons[1] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_RIGHT;
		} else {
			button_mask |= GLApp::BUTTON_RIGHT;
			glApp->buttonRelease(GLApp::BUTTON_RIGHT);
		}
	} else {
		if(state.rgbButtons[1] & 0x80) {
			glApp->mouse_button |= GLApp::BUTTON_RIGHT;
			glApp->buttonPress(GLApp::BUTTON_RIGHT);
		}
	}
	
	if(state.lZ > 0) glApp->mouse_button |= GLApp::BUTTON_UP;
	if(state.lZ < 0) glApp->mouse_button |= GLApp::BUTTON_DOWN;
}

/*
 */
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
	switch(message) {
		case WM_SIZE:
			if(wparam == SIZE_MINIMIZED) {
				minimized = 1;
				glApp->stopFps();
			} else {
				minimized = 0;
				glApp->startFps();
			}
			glApp->window_width = LOWORD(lparam);
			glApp->window_height = HIWORD(lparam);
			if(glApp->window_width < 1) glApp->window_width = 1;
			if(glApp->window_height < 1) glApp->window_height = 1;
			glViewport(0,0,glApp->window_width,glApp->window_height);
			return 1;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
				int key = translate_key((int)wparam);
				if(key) glApp->keyPress(key);
				if(key >= 'A' && key <= 'Z') key -= 'A' - 'a';
				for(int i = 0; i < num_keys; i++) {
					if(keys_buffer[i] == key) keys_buffer[i] = 0;
				}
				glApp->keys[key] = 1;
			}
			return 1;
		case WM_SYSKEYUP:
		case WM_KEYUP: {
				int key = translate_key((int)wparam);
				if(key) glApp->keyRelease(key);
				if(key >= 'A' && key <= 'Z') key -= 'A' - 'a';
				if(num_keys != sizeof(keys_buffer)) keys_buffer[num_keys++] = key;
				else glApp->keys[key] = 0;
			}
			return 1;
		case WM_CLOSE:
			glApp->exit();
			return 1;
		case WM_SETCURSOR:
			if(LOWORD(lparam) != HTCLIENT) break;
			SetCursor(cursor);
			return 1;
		case WM_LBUTTONDOWN:
			glApp->mouse_button |= GLApp::BUTTON_LEFT;
			glApp->buttonPress(GLApp::BUTTON_LEFT);
			return 1;
		case WM_LBUTTONUP:
			button_mask |= GLApp::BUTTON_LEFT;
			glApp->buttonRelease(GLApp::BUTTON_LEFT);
			return 1;
		case WM_MBUTTONDOWN:
			glApp->mouse_button |= GLApp::BUTTON_MIDDLE;
			glApp->buttonPress(GLApp::BUTTON_MIDDLE);
			return 1;
		case WM_MBUTTONUP:
			button_mask |= GLApp::BUTTON_MIDDLE;
			glApp->buttonRelease(GLApp::BUTTON_MIDDLE);
			return 1;
		case WM_RBUTTONDOWN:
			glApp->mouse_button |= GLApp::BUTTON_RIGHT;
			glApp->buttonPress(GLApp::BUTTON_RIGHT);
			return 1;
		case WM_RBUTTONUP:
			button_mask |= GLApp::BUTTON_RIGHT;
			glApp->buttonRelease(GLApp::BUTTON_RIGHT);
			return 1;
		case 0x020A: //WM_MOUSEWHEEL:
			if((short)HIWORD(wparam) > 0) {
				glApp->mouse_button |= GLApp::BUTTON_UP;
				glApp->buttonPress(GLApp::BUTTON_UP);
				glApp->buttonRelease(GLApp::BUTTON_UP);
			} else {
				glApp->mouse_button |= GLApp::BUTTON_DOWN;
				glApp->buttonPress(GLApp::BUTTON_DOWN);
				glApp->buttonRelease(GLApp::BUTTON_DOWN);
			}
			return 1;
		case WM_ACTIVATE:
			if(direct_mouse && show_mouse == 0) {
				if(WA_INACTIVE == wparam) direct_mouse->Unacquire();
				else direct_mouse->Acquire();
			}
			return 1;
		case WM_TIMER:
			read_direct_input();
			return 1;
	}
	return DefWindowProc(window,message,wparam,lparam);
}

/*
 */
void GLApp::setGamma(float value) {
	static WORD gamma[3][256];
	HDC hdc = GetDC(GetDesktopWindow());
	if(GetDeviceGammaRamp(hdc,gamma)) {
		for(int i = 0; i < 256; i++) {
			WORD g = 255 * (WORD)(255.0f * pow((float)i / 255.0f,1.0f / value));
			gamma[0][i] = g;
			gamma[1][i] = g;
			gamma[2][i] = g;
		}
		if(SetDeviceGammaRamp(hdc,gamma) == 0) {
			fprintf(stderr,"GLApp::setGamma(): SetDeviceGammaRamp() failed\n");
		}
	}
	ReleaseDC(GetDesktopWindow(),hdc);
}

/*
 */
int GLApp::setVideoMode(int w,int h,int flags) {
	
	if(window) {
		ReleaseDC(window,hdc);
		DestroyWindow(window);
		if((this->flags & FULLSCREEN) != (flags & FULLSCREEN)) {
			ChangeDisplaySettings(NULL,0);
			ShowCursor(true);
		}
	}
	
	window_width = w;
	window_height = h;
	this->flags = flags;
	
	mouse_button = 0;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	
	int attribs[128];
	memset(attribs,0,sizeof(attribs));
	int *aptr = attribs;
	
	*aptr++ = WGL_DRAW_TO_WINDOW_ARB;
	*aptr++ = GL_TRUE;
	*aptr++ = WGL_SUPPORT_OPENGL_ARB;
	*aptr++ = GL_TRUE;
	*aptr++ = WGL_ACCELERATION_ARB;
	*aptr++ = WGL_FULL_ACCELERATION_ARB;
	*aptr++ = WGL_DOUBLE_BUFFER_ARB;
	*aptr++ = GL_TRUE;
	*aptr++ = WGL_RED_BITS_ARB;
	*aptr++ = 8;
	*aptr++ = WGL_GREEN_BITS_ARB;
	*aptr++ = 8;
	*aptr++ = WGL_BLUE_BITS_ARB;
	*aptr++ = 8;
	*aptr++ = WGL_ALPHA_BITS_ARB;
	*aptr++ = 8;
	*aptr++ = WGL_DEPTH_BITS_ARB;
	*aptr++ = 24;
	*aptr++ = WGL_STENCIL_BITS_ARB;
	*aptr++ = 8;
	if(flags & MULTISAMPLE_2 || flags & MULTISAMPLE_4) {
		*aptr++ = WGL_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = WGL_SAMPLES_ARB;
		*aptr++ = (flags & MULTISAMPLE_2) ? 2 : 4;
	}
	
	try {
		
		HINSTANCE hInstance = GetModuleHandle(NULL);
		if(!window) {
			WNDCLASS wc;
			BYTE and_mask = 0xff;
			BYTE xor_mask = 0x00;
			wc.style = 0;
			wc.lpfnWndProc = (WNDPROC)window_proc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = hInstance;
			wc.hIcon = CreateIcon(hInstance,1,1,1,1,&and_mask,&xor_mask);
			wc.hCursor = LoadCursor(NULL,IDC_ARROW);
			wc.hbrBackground = NULL;
			wc.lpszMenuName = NULL;
			wc.lpszClassName = "GLApp";
			if(!RegisterClass(&wc)) throw("RegisterClass() failed");
		}

		if(flags & FULLSCREEN) {
			
			static int widths[] = { 320, 400, 640, 800, 1024, 1280, 1280, 1600 };
			static int heights[] = { 240, 300, 480, 600, 768, 960, 1024, 1200 };
			for(int i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
				if(widths[i] >= window_width && heights[i] >= window_height) {
					window_width = widths[i];
					window_height = heights[i];
					break;
				}
			}
			
			DEVMODE settings;
			memset(&settings,0,sizeof(DEVMODE));
			settings.dmSize = sizeof(DEVMODE);
			settings.dmPelsWidth = window_width;
			settings.dmPelsHeight = window_height;
			settings.dmBitsPerPel = 32;
			settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
			
			if(ChangeDisplaySettings(&settings,CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) throw("ChangeDisplaySettings() failed");
			
			window = CreateWindowEx(WS_EX_TOPMOST,"GLApp","GLApp",WS_POPUP | WS_VISIBLE,0,0,window_width,window_height,NULL,NULL,hInstance,NULL);
			
		} else {
			
			RECT windowRect = {0, 0, window_width, window_height };
			AdjustWindowRectEx(&windowRect,WS_OVERLAPPEDWINDOW,0,0);
			
			window_width = windowRect.right - windowRect.left;
			window_height = windowRect.bottom - windowRect.top;
			
			window = CreateWindowEx(0,"GLApp","GLApp",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,window_width,window_height,NULL,NULL,hInstance,NULL);
		}
		
		if(window == 0) throw("CreateWindowEx() failed");
		
		hdc = GetDC(window);
		if(hdc == 0) throw("GetDC() failed");
		
		// create simple opengl context
		if(wglChoosePixelFormatARB == NULL) {
			int pixelformat = ChoosePixelFormat(hdc,&pfd);
			SetPixelFormat(hdc,pixelformat,&pfd);
			
			// create context
			HGLRC old_context = context;
			context = wglCreateContext(hdc);
			if(context == NULL) throw("wglCreateContext() failed");
			if(old_context && !wglShareLists(old_context,context)) throw("wglShareLists() failed");
			if(wglMakeCurrent(hdc,context) == 0) throw("wglMakeCurrent() failed");
			
			// get proc address
			wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			if(wglChoosePixelFormatARB != NULL) {
				
				// destroy context
				wglMakeCurrent(hdc,0);
				wglDeleteContext(context);
				context = 0;
				
				// set video mode again
				return setVideoMode(w,h,flags);
			}
		}
		
		// create opengl context
		if(wglChoosePixelFormatARB != NULL) {
			int pixelformat;
			unsigned int count;
			wglChoosePixelFormatARB(hdc,attribs,NULL,1,&pixelformat,&count);
			if(count == 0) pixelformat = ChoosePixelFormat(hdc,&pfd);
			
			SetPixelFormat(hdc,pixelformat,&pfd);
			
			HGLRC old_context = context;
			context = wglCreateContext(hdc);
			if(context == NULL) throw("wglCreateContext() failed");
			if(old_context && wglShareLists(old_context,context) == 0) throw("wglShareLists() failed");
			if(wglMakeCurrent(hdc,context) == 0) throw("wglMakeCurrent() failed");
		}
		
		// wgl extensions
		if(wglGetExtensionsStringARB == NULL) wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
		if(wglGetExtensionsStringARB == NULL) wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
		
		glViewport(0,0,glApp->window_width,glApp->window_height);
		if((flags & MULTISAMPLE_2) || (flags & MULTISAMPLE_4)) glEnable(GL_MULTISAMPLE_ARB);
		else glDisable(GL_MULTISAMPLE_ARB);

		ShowWindow(window,SW_SHOW);
		UpdateWindow(window);
		SetForegroundWindow(window);
		SetFocus(window);
		
		SetWindowText(window,title);
		
		arrow_cursor = LoadCursor(NULL,IDC_ARROW);
		cursor = arrow_cursor;
		
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(window,&point);
		mouse_x = point.x;
		mouse_y = point.y;
		
		create_direct_input();
		
		show_mouse = !show_mouse;
		showMouse(!show_mouse);
	}
	catch(const char *error) {
		window = 0;
		exit(error);
		return 0;
	}
	
	// opengl info
	if(gl_info == NULL) gl_info = (const char*)glGetString(GL_RENDERER);
	if(gl_version == NULL) gl_version = (const char*)glGetString(GL_VERSION);
	if(gl_extensions == NULL) gl_extensions = (const char*)glGetString(GL_EXTENSIONS);
	if(wglx_extensions == NULL && wglGetExtensionsStringARB != NULL) wglx_extensions = (const char*)wglGetExtensionsStringARB(hdc);
	
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
void GLApp::setMouse(int x,int y) {
	mouse_x = x;
	mouse_y = y;
	POINT point;
	point.x = x;
	point.y = y;
	ClientToScreen(window,&point);
	SetCursorPos(point.x,point.y);
}

/*
 */
void GLApp::showMouse(int show) {
	if(show_mouse == show) return;
	show_mouse = show;
	if(show) cursor = arrow_cursor;
	else cursor = NULL;
	SetCursor(cursor);
	if(direct_mouse) {
		if(show) direct_mouse->Unacquire();
		else direct_mouse->Acquire();
		setMouse(mouse_x,mouse_y);
	}
}

/*
 */
int GLApp::checkExtension(const char *extension) {
	if(gl_extensions == NULL) return 0;
	if(strstr(gl_extensions,extension)) return 1;
	return 0;
}

/*
 */
void GLApp::error() {
	while(1) {
		GLenum error = glGetError();
		if(error == GL_NO_ERROR) break;
		fprintf(stderr,"OpenGL error 0x%04X: %s\n",error,gluErrorString(error));
	}
}

/*
 */
void GLApp::exit(const char *error,...) {
	showMouse(1);
	if(error) {
		char buf[1024];
		va_list arg;
		va_start(arg,error);
		_vsnprintf(buf,sizeof(buf),error,arg);
		va_end(arg);
		MessageBox(0,buf,"GLApp::exit():",MB_OK);
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
	return GetOpenFileName(&ofn);
}

/*
 */
static long long get_time() {
	static int init = 0;
	static LARGE_INTEGER freq;
	if(init == 0) {
		QueryPerformanceFrequency(&freq);
		init = 1;
	}
	LARGE_INTEGER tval;
	QueryPerformanceCounter(&tval);
	return (long long)(tval.QuadPart * 1000000 / freq.QuadPart);
}

/*
 */
void GLApp::stopFps() {
	fps_delta = get_time() - fps_time;
}

/*
 */
void GLApp::startFps() {
	fps_time = get_time() - fps_delta;
}

/*
 */
void GLApp::update() {
	
	// release keys
	for(int i = 0; i < num_keys; i++) {
		if(keys_buffer[i] == 0) continue;
		keys[keys_buffer[i]] = 0;
	}
	num_keys = 0;
	
	// release mouse button and get mouse position
	if(show_mouse == 1) {
		if((mouse_button & BUTTON_LEFT) && GetAsyncKeyState(VK_LBUTTON) == 0) {
			button_mask |= BUTTON_LEFT;
			glApp->buttonRelease(GLApp::BUTTON_LEFT);
		}
		if((mouse_button & BUTTON_MIDDLE) && GetAsyncKeyState(VK_MBUTTON) == 0) {
			button_mask |= BUTTON_MIDDLE;
			glApp->buttonRelease(GLApp::BUTTON_MIDDLE);
		}
		if((mouse_button & BUTTON_RIGHT) && GetAsyncKeyState(VK_RBUTTON) == 0) {
			button_mask |= BUTTON_RIGHT;
			glApp->buttonRelease(GLApp::BUTTON_RIGHT);
		}
		
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(window,&point);
		mouse_x = point.x;
		mouse_y = point.y;
	}
	
	mouse_button &= ~button_mask;
	button_mask = 0;
	
	// process messages
	MSG msg;
	while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE) || minimized == 1) {
		if(!GetMessage(&msg,NULL,0,0)) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	if(fps_init == 0) {
		fps_time = get_time();
		fps_counter = 0;
		fps_init = 1;
	}
	
	if(fps_counter++ == 10) {
		long long fps_old_time = fps_time;
		fps_time = get_time(); 
		fps = (float)((double)fps_counter * 1000000.0 / (double)(fps_time - fps_old_time));
		fps_counter = 0;
	}
	ifps = 1.0f / fps;
}

/*
 */
void GLApp::swap() {
	SwapBuffers(wglGetCurrentDC());
	mouse_button &= ~(BUTTON_UP | BUTTON_DOWN);
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

#ifdef WINMAIN

/*
 */
extern int main(int argc,char **argv);

/*
 */
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {
	
	int argc = 1;
	char *argv[256];
	argv[0] = "none";
	while(*lpCmdLine && argc < 256) {
		while(*lpCmdLine && (*lpCmdLine <= ' ' || *lpCmdLine == '"')) lpCmdLine++;
		if(*lpCmdLine) {
			if(*lpCmdLine == '"') lpCmdLine++;
			argv[argc++] = lpCmdLine;
			while(*lpCmdLine && *lpCmdLine > ' ' && *lpCmdLine != '"') lpCmdLine++;
			if(*lpCmdLine) *(lpCmdLine++) = '\0';
		}
	}
	
	FILE *file = fopen("stderr.txt","wb");
	if(file) {
		setbuf(file,NULL);
		*stderr = *file;
	}
	
	main(argc,argv);
	
	if(file) {
		fclose(file);
	}
	
	file = fopen("stderr.txt","rb");
	if(file) {
		char c = fgetc(file);
		fclose(file);
		if(c == EOF) remove("stderr.txt");
	}
	
	return 0;
}

#endif /* WINMAIN */

#endif
