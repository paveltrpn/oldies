/*	OpenGL pixel buffer
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef _WIN32

/* linux
 */
#include <iostream>
#include <GL/gl.h>
#include <GL/glx.h>
#include "pbuffer.h"

/*
 */
struct PBuffer_data {
	Display *display;
	GLXPbuffer pbuffer;
	GLXContext context;
	
	Display *old_display;
	GLXPbuffer old_pbuffer;
	GLXContext old_context;
};

/*
 */
PBuffer::PBuffer(int width,int height,int float_buffer) : width(width), height(height) {
	
	Display *display = glXGetCurrentDisplay();
	int screen = DefaultScreen(display);
	GLXContext old_context = glXGetCurrentContext();
	
	int attrib[] = {
		GLX_RED_SIZE,8,
		GLX_GREEN_SIZE,8,
		GLX_BLUE_SIZE,8,
		GLX_ALPHA_SIZE,8,
		GLX_STENCIL_SIZE,8,
		GLX_DEPTH_SIZE,24,
		GLX_DRAWABLE_TYPE,GLX_PBUFFER_BIT,
		0
	};
	int attrib_float[] = {
		GLX_RED_SIZE,32,
		GLX_GREEN_SIZE,32,
		GLX_BLUE_SIZE,32,
		GLX_ALPHA_SIZE,32,
		GLX_STENCIL_SIZE,8,
		GLX_DEPTH_SIZE,24,
		GLX_FLOAT_COMPONENTS_NV,true,
		GLX_DRAWABLE_TYPE,GLX_PBUFFER_BIT,
		0
	};
	
	int count;
	GLXFBConfig *config;
	if(float_buffer) config = glXChooseFBConfigSGIX(display,screen,attrib_float,&count);
	else config = glXChooseFBConfigSGIX(display,screen,attrib,&count);	
	if(!config) {
		fprintf(stderr,"pbuffer: glXChooseFBConfigSGIX() failed\n");
		return;
	}
	
	int pattrib[] = {
		GLX_LARGEST_PBUFFER,true,
		GLX_PRESERVED_CONTENTS,true,
		0
	};

	GLXPbuffer pbuffer = glXCreateGLXPbufferSGIX(display,config[0],width,height,pattrib);
	if(!pbuffer) {
		fprintf(stderr,"pbuffer: glXCreateGLXPbufferSGIX() failed\n");
		return;
	}

	GLXContext context = glXCreateContextWithConfigSGIX(display,config[0],GLX_RGBA_TYPE,old_context,true);
	if(!context) {
		fprintf(stderr,"pbuffer: glXCreateContextWithConfigSGIX() failed\n");
		return;
	}
	
	data = new PBuffer_data;
	data->display = display;
	data->pbuffer = pbuffer;
	data->context = context;

	data->old_display = display;
	data->old_pbuffer = glXGetCurrentDrawable();
	data->old_context = old_context;
}

/*
 */
PBuffer::~PBuffer() {
	if(data->context) glXDestroyContext(data->display,data->context);
	if(data->pbuffer) glXDestroyGLXPbufferSGIX(data->display,data->pbuffer);
	delete data;
}

/*
 */
int PBuffer::enable() {
	if(!glXMakeCurrent(data->display,data->pbuffer,data->context)) {
		fprintf(stderr,"pbuffer: glXMakeCurrent() failed\n");
		return -1;
	}
	return 0;
}

/*
 */
int PBuffer::disable() {
	if(!glXMakeCurrent(data->old_display,data->old_pbuffer,data->old_context)) {
		fprintf(stderr,"pbuffer: glXMakeCurrent() failed\n");
		return -1;
	}
	return 0;
}

#else

/* windows
 */
#include <iostream>
#include <windows.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include "pbuffer.h"

/*
 */
struct PBuffer_data {
	HDC hdc;
	HPBUFFERARB pbuffer;
	HGLRC context;
	
	HDC old_hdc;
	HGLRC old_context;
};

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = 0;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = 0;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = 0;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = 0;

/*
 */
PBuffer::PBuffer(int width,int height,int float_buffer) : width(width), height(height) {
	
	if(!wglChoosePixelFormatARB) wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if(!wglChoosePixelFormatARB) {
		fprintf(stderr,"pbuffer: wglGetProcAddress(\"wglChoosePixelFormatARB\") failed\n");
		return;
	}
	if(!wglCreatePbufferARB) wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
	if(!wglCreatePbufferARB) {
		fprintf(stderr,"pbuffer: wglGetProcAddress(\"wglCreatePbufferARB\") failed\n");
		return;
	}
	if(!wglGetPbufferDCARB) wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
	if(!wglGetPbufferDCARB) {
		fprintf(stderr,"pbuffer: wglGetProcAddress(\"wglGetPbufferDCARB\") failed\n");
		return;
	}
	if(!wglReleasePbufferDCARB) wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
	if(!wglReleasePbufferDCARB) {
		fprintf(stderr,"pbuffer: wglGetProcAddress(\"wglReleasePbufferDCARB\") failed\n");
		return;
	}
	if(!wglDestroyPbufferARB) wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
	if(!wglDestroyPbufferARB) {
		fprintf(stderr,"pbuffer: wglGetProcAddress(\"wglDestroyPbufferARB\") failed\n");
		return;
	}

	HDC old_hdc = wglGetCurrentDC();
	HGLRC old_context = wglGetCurrentContext();
	
	int attrib[] = {
		WGL_RED_BITS_ARB,8,
		WGL_GREEN_BITS_ARB,8,
		WGL_BLUE_BITS_ARB,8,
		WGL_ALPHA_BITS_ARB,8,
		WGL_STENCIL_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,24,
		WGL_DRAW_TO_PBUFFER_ARB,true,
		WGL_SUPPORT_OPENGL_ARB,true,
		WGL_PIXEL_TYPE_ARB,WGL_TYPE_RGBA_ARB,
		0
	};
	int attrib_float[] = {
		WGL_RED_BITS_ARB,32,
		WGL_GREEN_BITS_ARB,32,
		WGL_BLUE_BITS_ARB,32,
		WGL_ALPHA_BITS_ARB,32,
		WGL_STENCIL_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,24,
		WGL_FLOAT_COMPONENTS_NV,true,
		WGL_DRAW_TO_PBUFFER_ARB,true,
		WGL_SUPPORT_OPENGL_ARB,true,
		WGL_PIXEL_TYPE_ARB,WGL_TYPE_RGBA_ARB,
		0
	};

	int format;
	unsigned int count;
	if(float_buffer) wglChoosePixelFormatARB(old_hdc,attrib_float,NULL,1,&format,&count);
	else wglChoosePixelFormatARB(old_hdc,attrib,NULL,1,&format,&count);
	if(count == 0) {
		fprintf(stderr,"pbuffer: wglChoosePixelFormatARB() failed\n");
		return;
	}

	int pattrib[] = { 0 };

	HPBUFFERARB pbuffer = wglCreatePbufferARB(old_hdc,format,width,height,pattrib);
	if(!pbuffer) {
		fprintf(stderr,"pbuffer: wglCreatePbufferARB() failed\n");
		return;
	}

	HDC hdc = wglGetPbufferDCARB(pbuffer);
	if(!hdc) {
		fprintf(stderr,"pbuffer: wglGetPbufferDCARB() failed\n");
		return;
	}

	HGLRC context = wglCreateContext(hdc);
	if(!context) {
		fprintf(stderr,"pbuffer: wglCreateContext() failed\n");
		return;
	}

	if(!wglShareLists(old_context,context)) {
		fprintf(stderr,"pbuffer: wglShareLists() failed\n");
		return;
	}
	
	data = new PBuffer_data;
	data->hdc = hdc;
	data->pbuffer = pbuffer;
	data->context = context;

	data->old_hdc = old_hdc;
	data->old_context = old_context;
}

/*
 */
PBuffer::~PBuffer() {
	wglDeleteContext(data->context);
	wglReleasePbufferDCARB(data->pbuffer,data->hdc);
	wglDestroyPbufferARB(data->pbuffer);
	wglMakeCurrent(data->hdc,data->context);
}

/*
 */
int PBuffer::enable() {
	wglMakeCurrent(data->hdc,data->context);
	return 0;
}

/*
 */
int PBuffer::disable() {
	wglMakeCurrent(data->old_hdc,data->old_context);
	return 0;
}

#endif
