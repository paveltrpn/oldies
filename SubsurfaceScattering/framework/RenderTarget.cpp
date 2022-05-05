/* OpenGL Render Target
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
#include <GL/gl.h>
#include <GL/glx.h>

#include "Texture.h"
#include "RenderTarget.h"

/*
 */
struct RenderTargetData {
	
	Display *display;
	
	GLXPbuffer pbuffer;
	GLXContext context;
	
	GLXPbuffer old_pbuffer;
	GLXContext old_context;
};

/*
 */
RenderTarget::RenderTarget(int width,int height,int flags) {
	
	this->width = width;
	this->height = height;
	this->flags = flags;
	
	data = NULL;
	target = NULL;
	face = 0;
	
	int bits = 8;
	if(flags & FLOAT_16) bits = 16;
	if(flags & FLOAT_32) bits = 32;
	
	int attribs[128];
	memset(attribs,0,sizeof(attribs));
	int *aptr = attribs;
	
	*aptr++ = GLX_RENDER_TYPE;
	*aptr++ = GLX_RGBA_BIT;
	*aptr++ = GLX_DRAWABLE_TYPE;
	*aptr++ = GLX_PBUFFER_BIT;
	*aptr++ = GLX_RED_SIZE;
	*aptr++ = bits;
	*aptr++ = GLX_GREEN_SIZE;
	*aptr++ = bits;
	*aptr++ = GLX_BLUE_SIZE;
	*aptr++ = bits;
	*aptr++ = GLX_ALPHA_SIZE;
	*aptr++ = bits;
	*aptr++ = GLX_DEPTH_SIZE;
	*aptr++ = 24;
	*aptr++ = GLX_STENCIL_SIZE;
	*aptr++ = 8;
	if((flags & FLOAT_16) || (flags & FLOAT_32)) {
		*aptr++ = GLX_FLOAT_COMPONENTS_NV;
		*aptr++ = GL_TRUE;
	}
	if(flags & MULTISAMPLE_2) {
		*aptr++ = GLX_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = GLX_SAMPLES_ARB;
		*aptr++ = 2;
	}
	else if(flags & MULTISAMPLE_4) {
		*aptr++ = GLX_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = GLX_SAMPLES_ARB;
		*aptr++ = 4;
	}
	
	int pattribs[128];
	memset(pattribs,0,sizeof(pattribs));
	int *paptr = pattribs;
	
	*paptr++ = GLX_LARGEST_PBUFFER;
	*paptr++ = GL_FALSE;
	
	Display *display = glXGetCurrentDisplay();
	int screen = DefaultScreen(display);
	GLXContext old_context = glXGetCurrentContext();
	
	GLXPbuffer pbuffer;
	GLXContext context;
	
	try {
		
		const char *extensions = (const char*)glXQueryExtensionsString(display,screen);
		
		if(strstr(extensions,"GLX_SGIX_pbuffer") && strstr(extensions,"GLX_SGIX_fbconfig")) {
			
			int count;
			GLXFBConfig *config = glXChooseFBConfigSGIX(display,screen,attribs,&count);
			if(config == NULL) throw("glXChooseFBConfigSGIX() failed");
			
			pbuffer = glXCreateGLXPbufferSGIX(display,config[0],width,height,pattribs);
			if(pbuffer == 0) throw("glXCreateGLXPbufferSGIX() failed");
			
			context = glXCreateContextWithConfigSGIX(display,config[0],GLX_RGBA_TYPE,old_context,true);
			if(context == NULL) throw("glXCreateContextWithConfigSGIX() failed");
			
			unsigned int w,h;
			glXQueryGLXPbufferSGIX(display,pbuffer,GLX_WIDTH,&w);
			glXQueryGLXPbufferSGIX(display,pbuffer,GLX_HEIGHT,&h);
			this->width = w;
			this->height = h;
			
			XFree(config);
		}
		else {
			
			*paptr++ = GLX_PBUFFER_WIDTH;
			*paptr++ = width;
			*paptr++ = GLX_PBUFFER_HEIGHT;
			*paptr++ = height;
			
			int count;
			GLXFBConfig *config = glXChooseFBConfig(display,screen,attribs,&count);	
			if(config == NULL) throw("glXChooseFBConfig() failed");
			
			pbuffer = glXCreatePbuffer(display,config[0],pattribs);
			if(pbuffer == 0) throw("glXCreatePbuffer() failed");
			
			XVisualInfo *visual = glXGetVisualFromFBConfig(display,config[0]);
			if(visual == NULL) throw("glXGetVisualFromFBConfig() failed");
			
			context = glXCreateContext(display,visual,old_context,true);
			if(context == NULL) throw("glXCreateContext() failed");
			
			XFree(config);
			XFree(visual);
		}
	}
	catch(const char *error) {
		fprintf(stderr,"RenderTarget::RenderTarget(%dx%d): %s\n",width,height,error);
		return;
	}
	
	data = new RenderTargetData;
	data->display = display;
	
	data->pbuffer = pbuffer;
	data->context = context;
	
	data->old_pbuffer = glXGetCurrentDrawable();
	data->old_context = old_context;
}

/*
 */
RenderTarget::~RenderTarget() {
	if(data) {
		if(data->context) glXDestroyContext(data->display,data->context);
		if(data->pbuffer) glXDestroyPbuffer(data->display,data->pbuffer);
		delete data;
	}
}

/*
 */
void RenderTarget::setTarget(Texture *texture) {
	target = NULL;
	if(data == NULL) return;
	if(texture == NULL) return;
	if(texture->width > width || texture->height > height) {
		fprintf(stderr,"RenderTarget::setTarget(): bad Texture size (%dx%d) for (%dx%d) Render Target\n",texture->width,texture->height,width,height);
		return;
	}
	target = texture;
	if(target) {
		GLXPbuffer pbuffer = glXGetCurrentDrawable();
		GLXContext context = glXGetCurrentContext();
		if(pbuffer == data->pbuffer && context == data->context) {
			glEnable(GL_SCISSOR_TEST);
			glScissor(0,0,target->width,target->height);
			glViewport(0,0,target->width,target->height);
		}
	}
}

/*
 */
void RenderTarget::setFace(int face) {
	if(data == NULL) return;
	if(target == NULL) return;
	if(face < 0 || face > 5) {
		fprintf(stderr,"RenderTarget::setFace(): bad face %d\n",face);
		return;
	}
	this->face = face;
}

/*
 */
void RenderTarget::flush() {
	if(data == NULL) return;
	if(target == NULL) return;
	if(target->target == Texture::TEXTURE_2D || target->target == Texture::TEXTURE_RECT) {
		target->bind(0);
		target->copy();
	} else if(target->target == Texture::TEXTURE_CUBE) {
		target->bind(0);
		target->copy(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
	} else {
		fprintf(stderr,"RenderTarget::flush(): unknown type of texture\n");
	}
}

/*
 */
void RenderTarget::enable() {
	if(data == NULL) return;
	data->old_pbuffer = glXGetCurrentDrawable();
	data->old_context = glXGetCurrentContext();
	if(data->old_pbuffer == data->pbuffer || data->old_context == data->context) {
		fprintf(stderr,"RenderTarget::enable(): it's the same context\n");
		return;
	}
	if(glXMakeCurrent(data->display,data->pbuffer,data->context) == 0) {
		fprintf(stderr,"RenderTarget::enable(): glXMakeCurrent() failed\n");
		return;
	}
	if(target) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(0,0,target->width,target->height);
		glViewport(0,0,target->width,target->height);
	}
}

/*
 */
void RenderTarget::disable() {
	if(data == NULL) return;
	glDisable(GL_SCISSOR_TEST);
	glScissor(0,0,width,height);
	glViewport(0,0,width,height);
	if(glXMakeCurrent(data->display,data->old_pbuffer,data->old_context) == 0) {
		fprintf(stderr,"RenderTarget::disable(): glXMakeCurrent() failed\n");
		return;
	}
}

#else

/* windows
 */
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include "Texture.h"
#include "RenderTarget.h"

/*
 */
struct RenderTargetData {
	
	HDC hdc;
	HPBUFFERARB pbuffer;
	HGLRC context;
	
	HDC old_hdc;
	HGLRC old_context;
};

static int have_arb_render_texture = 0;
static int have_nv_render_texture_rectangle = 0;

static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = NULL;
static PFNWGLSETPBUFFERATTRIBARBPROC wglSetPbufferAttribARB = NULL;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = NULL;
static PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB = NULL;
static PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB = NULL;
static PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB = NULL;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = NULL;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = NULL;

/*
 */
RenderTarget::RenderTarget(int width,int height,int flags) {
	
	// initialize render target
	this->width = width;
	this->height = height;
	this->flags = flags;
	
	data = NULL;
	target = NULL;
	face = 0;

	// init wgl extensions
	try {
		if(wglGetExtensionsStringARB == NULL) {
			if(wglGetExtensionsStringARB == NULL) wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			if(wglGetExtensionsStringARB == NULL) wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
			if(wglGetExtensionsStringARB == NULL) throw("wglGetProcAddress(\"wglGetExtensionsStringARB\") failed");
			if(wglChoosePixelFormatARB == NULL) wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			if(wglChoosePixelFormatARB == NULL) throw("wglGetProcAddress(\"wglChoosePixelFormatARB\") failed");
			if(wglCreatePbufferARB == NULL) wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
			if(wglCreatePbufferARB == NULL) throw("wglGetProcAddress(\"wglCreatePbufferARB\") failed");
			if(wglSetPbufferAttribARB == NULL) wglSetPbufferAttribARB = (PFNWGLSETPBUFFERATTRIBARBPROC)wglGetProcAddress("wglSetPbufferAttribARB");
			if(wglSetPbufferAttribARB == NULL) throw("wglGetProcAddress(\"wglSetPbufferAttribARB\") failed");
			if(wglGetPbufferDCARB == NULL) wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
			if(wglGetPbufferDCARB == NULL) throw("wglGetProcAddress(\"wglGetPbufferDCARB\") failed");
			if(wglBindTexImageARB == NULL) wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC)wglGetProcAddress("wglBindTexImageARB");
			if(wglBindTexImageARB == NULL) throw("wglGetProcAddress(\"wglBindTexImageARB\") failed");
			if(wglReleaseTexImageARB == NULL) wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC)wglGetProcAddress("wglReleaseTexImageARB");
			if(wglReleaseTexImageARB == NULL) throw("wglGetProcAddress(\"wglReleaseTexImageARB\") failed");
			if(wglQueryPbufferARB == NULL) wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)wglGetProcAddress("wglQueryPbufferARB");
			if(wglQueryPbufferARB == NULL) throw("wglGetProcAddress(\"wglQueryPbufferARB\") failed");
			if(wglReleasePbufferDCARB == NULL) wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
			if(wglReleasePbufferDCARB == NULL) throw("wglGetProcAddress(\"wglReleasePbufferDCARB\") failed\n");
			if(wglDestroyPbufferARB == NULL) wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
			if(wglDestroyPbufferARB == NULL) throw("wglGetProcAddress(\"wglDestroyPbufferARB\") failed\n");
			
			// check extensions
			const char *extensions = (const char*)wglGetExtensionsStringARB(wglGetCurrentDC());
			have_arb_render_texture = strstr(extensions,"WGL_ARB_render_texture") ? 1 : 0;
			have_nv_render_texture_rectangle = strstr(extensions,"WGL_NV_render_texture_rectangle") ? 1 : 0;
		}
	}
	catch(const char *error) {
		fprintf(stderr,"RenderTarget::RenderTarget(): %s\n",error);
		return;
	}
	
	// create pbuffer
	int bits = 8;
	if(flags & FLOAT_16) bits = 16;
	if(flags & FLOAT_32) bits = 32;
	
	int attribs[128];
	memset(attribs,0,sizeof(attribs));
	int *aptr = attribs;	
	
	*aptr++ = WGL_PIXEL_TYPE_ARB;
	*aptr++ = (flags & FLOAT_16) ? WGL_TYPE_RGBA_FLOAT_ATI : WGL_TYPE_RGBA_ARB;
	*aptr++ = WGL_DRAW_TO_PBUFFER_ARB;
	*aptr++ = GL_TRUE;
	*aptr++ = WGL_SUPPORT_OPENGL_ARB;
	*aptr++ = GL_TRUE;
	*aptr++ = WGL_DOUBLE_BUFFER_ARB;
	*aptr++ = GL_FALSE;
	*aptr++ = WGL_RED_BITS_ARB;
	*aptr++ = bits;
	*aptr++ = WGL_GREEN_BITS_ARB;
	*aptr++ = bits;
	*aptr++ = WGL_BLUE_BITS_ARB;
	*aptr++ = bits;
	*aptr++ = WGL_ALPHA_BITS_ARB;
	*aptr++ = bits;
	*aptr++ = WGL_DEPTH_BITS_ARB;
	*aptr++ = 24;
	*aptr++ = WGL_STENCIL_BITS_ARB;
	*aptr++ = 8;
	if(flags & FLOAT_32) {
		*aptr++ = WGL_FLOAT_COMPONENTS_NV;
		*aptr++ = GL_TRUE;
	}
	if(flags & MULTISAMPLE_2) {
		*aptr++ = WGL_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = WGL_SAMPLES_ARB;
		*aptr++ = 2;
	}
	else if(flags & MULTISAMPLE_4) {
		*aptr++ = WGL_SAMPLE_BUFFERS_ARB;
		*aptr++ = GL_TRUE;
		*aptr++ = WGL_SAMPLES_ARB;
		*aptr++ = 4;
	}
	
	int pattribs[128];
	memset(pattribs,0,sizeof(pattribs));
	int *paptr = pattribs;
	
	*paptr++ = WGL_PBUFFER_LARGEST_ARB;
	*paptr++ = GL_FALSE;
	
	HDC old_hdc = wglGetCurrentDC();
	HGLRC old_context = wglGetCurrentContext();
	
	HDC hdc;
	HPBUFFERARB pbuffer;
	HGLRC context;
	
	try {
		int pixelformat;
		unsigned int count;
		wglChoosePixelFormatARB(old_hdc,attribs,NULL,1,&pixelformat,&count);
		if(count == 0) throw("wglChoosePixelFormatARB() failed");
		
		pbuffer = wglCreatePbufferARB(old_hdc,pixelformat,width,height,pattribs);
		if(pbuffer == 0) throw("wglCreatePbufferARB() failed");
		
		hdc = wglGetPbufferDCARB(pbuffer);
		if(hdc == 0) throw("wglGetPbufferDCARB() failed");
		
		context = wglCreateContext(hdc);
		if(context == NULL) throw("wglCreateContext() failed");
		
		if(wglShareLists(old_context,context) == 0) throw("wglShareLists() failed");
	}
	catch(const char *error) {
		fprintf(stderr,"RenderTarget::RenderTarget(%dx%d): %s\n",width,height,error);
		return;
	}
	
	data = new RenderTargetData;
	data->hdc = hdc;
	data->pbuffer = pbuffer;
	data->context = context;
	
	data->old_hdc = old_hdc;
	data->old_context = old_context;
}

/*
 */
RenderTarget::~RenderTarget() {
	if(data) {
		wglDeleteContext(data->context);
		wglReleasePbufferDCARB(data->pbuffer,data->hdc);
		wglDestroyPbufferARB(data->pbuffer);
		delete data;
	}
}

/*
 */
void RenderTarget::setTarget(Texture *texture) {
	target = NULL;
	if(data == NULL) return;
	if(texture == NULL) return;
	if(texture->width > width || texture->height > height) {
		fprintf(stderr,"RenderTarget::setTarget(): bad Texture size (%dx%d) for (%dx%d) Render Target\n",texture->width,texture->height,width,height);
		return;
	}
	target = texture;
	if(target) {
		HDC hdc = wglGetCurrentDC();
		HGLRC context = wglGetCurrentContext();
		if(hdc == data->hdc && context == data->context) {
			glEnable(GL_SCISSOR_TEST);
			glScissor(0,0,target->width,target->height);
			glViewport(0,0,target->width,target->height);
		}
	}
}

/*
 */
void RenderTarget::setFace(int face) {
	if(data == NULL) return;
	if(target == NULL) return;
	if(face < 0 || face > 5) {
		fprintf(stderr,"RenderTarget::setFace(): bad face %d\n",face);
		return;
	}
	this->face = face;
}

/*
 */
void RenderTarget::flush() {
	if(data == NULL) return;
	if(target == NULL) return;
	if(target->target == Texture::TEXTURE_2D || target->target == Texture::TEXTURE_RECT) {
		target->bind(0);
		target->copy();
	} else if(target->target == Texture::TEXTURE_CUBE) {
		target->bind(0);
		target->copy(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
	} else {
		fprintf(stderr,"RenderTarget::flush(): unknown type of texture\n");
	}
}

/*
 */
void RenderTarget::enable() {
	if(data == NULL) return;
	data->old_hdc = wglGetCurrentDC();
	data->old_context = wglGetCurrentContext();
	if(data->old_hdc == data->hdc || data->old_context == data->context) {
		fprintf(stderr,"RenderTarget::enable(): it's the same context\n");
		return;
	}
	if(wglMakeCurrent(data->hdc,data->context) == 0) {
		fprintf(stderr,"RenderTarget::enable(): wglMakeCurrent() failed\n");
		return;
	}
	if(target) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(0,0,target->width,target->height);
		glViewport(0,0,target->width,target->height);
	}
}

/*
 */
void RenderTarget::disable() {
	if(data == NULL) return;
	glScissor(0,0,width,height);
	glViewport(0,0,width,height);
	if(wglMakeCurrent(data->old_hdc,data->old_context) == 0) {
		fprintf(stderr,"RenderTarget::disable(): wglMakeCurrent() failed\n");
		return;
	}
}

#endif
