/* Subsurface Scattering
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GLApp.h"
#include "GLExt.h"
#include "RenderTarget.h"
#include "MathLib.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Image.h"
#include "Font.h"

#define DEPTH	256

/*
 */
class GLAppMain : public GLApp {
		
	public:
		
		GLAppMain() { }
		virtual ~GLAppMain() { }
		
		void init();
		void idle();
		
		void create_depends(float radius);
		
		void render_light();
		void render_scattering();
		void render_extrude();
		
		void render_particle(const vec3 &pos,float radius,float angle);
		void render_flare(const vec3 &pos,const vec3 &color,float radius);
		
		void render();
		
		float time;
		int pause;
		int scattering;
		
		vec3 pos;
		vec3 dir;
		vec3 up;
		int old_mouse_x;
		int old_mouse_y;
		
		mat4 projection;
		mat4 modelview;
		
		vec3 light_pos;
		vec3 light_color;
		
		Shader *depends_shader;
		Shader *light_shader;
		Shader *scattering_shader;
		Shader *extrude_shader;
		Shader *final_shader;
		
		int size;
		
		RenderTarget *light_rt;
		Texture *light_tex;
		
		GLuint depends_tex_id;
		
		Mesh *mesh;
		
		Texture *flare_tex;
		
		Font *font;
};

/*
 */
void GLAppMain::init() {
	
	// check hardware
	if(checkExtension("GL_ARB_vertex_buffer_object") == 0 ||
		checkExtension("GL_ARB_shader_objects") == 0 ||
		checkExtension("GL_ARB_vertex_shader") == 0 ||
		checkExtension("GL_ARB_fragment_shader") == 0) {
		
		exit("upgrade your driver or video card\n");
		return;
	}
	
	// dynamic branching ;)
	if(checkExtension("GL_NV_fragment_program2") == 0) {
		exit("GeForce 6 series is required\n");
		return;
	}
	
	// init extensions
	glext_init();
	
	// variables
	time = 0.0f;
	pause = 0;
	scattering = 1;
	
	pos = vec3(-2.87f,1.84f,1.95f);
	dir = vec3(0,0,0);
	up = vec3(0,0,1);
	
	light_color = vec3(0.9f,0.9f,1.2f);
	
	// load shaders
	depends_shader = new Shader("data/shaders/depends.shader");
	light_shader = new Shader("data/shaders/light.shader");
	scattering_shader = new Shader("data/shaders/scattering.shader");
	extrude_shader = new Shader("data/shaders/extrude.shader");
	final_shader = new Shader("data/shaders/final.shader");
	
	// lighting
	size = 128;
	
	light_rt = new RenderTarget(size,size);
	light_tex = new Texture(size,size,Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR);
	
	// mesh
	mesh = new Mesh("data/mesh.mesh");
	
	// flares
	flare_tex = new Texture("data/flare.png",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_LINEAR | Texture::CLAMP);
	
	font = new Font("data/font.png");
	
	// depends texture
	create_depends(0.4f);
	
	error();
}

/*
 */
void GLAppMain::create_depends(float radius) {
	
	// try to load precomputed texture
	int width,height,depth;
	unsigned char *data = Image::load_3d("data/lockup.3d",width,height,depth);
	if(data) {
		
		if(width == size && height == size && depth == DEPTH) {
			glGenTextures(1,&depends_tex_id);
			glBindTexture(GL_TEXTURE_3D,depends_tex_id);
			glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA,size,size,DEPTH,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
			
			delete [] data;
			return;
		}
		
		delete [] data;
	}
	
	float *coords = new float[size * size * 4];
	
	// create arrays of coordinates
	RenderTarget rt(size,size,RenderTarget::FLOAT_32);
	rt.enable();
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
		
		depends_shader->enable();
		depends_shader->bind();
		mesh->render();
		depends_shader->disable();
		
		glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,coords);
	rt.disable();
	
	// find nearest points
	struct NearPoint {
		int s,t;
		float dist;
		float k;
	};
	
	struct Point {
		int num_points;
		NearPoint points[DEPTH];
	};
	
	Point *points = new Point[size * size];
	
	font->enable(1024,768);
	
	for(int y = 0; y < size; y++) {
		for(int x = 0; x < size; x++) {
			
			int i = (size * y + x) << 2;
			if(coords[i + 3] < EPSILON) continue;
			vec3 pos = vec3(&coords[i]);
			
			Point *point = &points[size * y + x];
			point->num_points = 0;
			
			for(int t = 0; t < size; t++) {
				for(int s = 0; s < size; s++) {
					int j = (size * t + s) << 2;
					if(coords[j + 3] < EPSILON) continue;
					vec3 p = vec3(&coords[j]);
					
					float dist = length(pos - p);
					if(dist > radius) continue;
					
					// add new point
					if(point->num_points < DEPTH) {
						NearPoint *np = &point->points[point->num_points++];
						np->s = s;
						np->t = t;
						np->dist = dist;
					}
					// random point
					else {
						NearPoint *np = &point->points[rand() % DEPTH];
						np->s = s;
						np->t = t;
						np->dist = dist;
					}
				}
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		int size_x,size_y;
		font->getSize(22,"create lockup texture",size_x,size_y);
		font->printf(1024 / 2 - size_x / 2,768 / 2 - size_y / 2 - 22,22,"Create lockup texture");
		font->getSize(22,"50 percent complete",size_x,size_y);
		font->printf(1024 / 2 - size_x / 2,768 / 2 - size_y / 2,22,"%.0f percent complete",(float)y / (float)size * 100.0f);
		swap();
	}
	
	font->disable();
	
	delete [] coords;
	
	// create 3d lockup texture
	data = new unsigned char[size * size * DEPTH * 4];
	memset(data,0,sizeof(unsigned char) * size * size * DEPTH * 4);
	
	for(int y = 0; y < size; y++) {
		for(int x = 0; x < size; x++) {
			
			Point *p = &points[size * y + x];
			if(p->num_points == 0) continue;
			
			float k = 0;
			for(int i = 0; i < p->num_points; i++) {
				NearPoint *np = &p->points[i];
				np->k = exp(-np->dist * radius * 3.5f);
				k += np->k;
			}
			for(int i = 0; i < p->num_points; i++) {
				NearPoint *np = &p->points[i];
				np->k = np->k / (k / 2.0f);
			}
			
			for(int i = 0; i < p->num_points; i++) {
				NearPoint *np = &p->points[i];
				unsigned char *d = &data[(size * size * i + size * y + x) << 2];
				*d++ = (unsigned char)((float)np->s / (float)size * 255.0f);
				*d++ = (unsigned char)((float)np->t / (float)size * 255.0f);
				*d++ = (unsigned char)(np->k * 255.0f);
				*d++ = 255;
			}
		}
	}
	
	Image::save_3d("data/lockup.3d",data,size,size,DEPTH);
	
	glGenTextures(1,&depends_tex_id);
	glBindTexture(GL_TEXTURE_3D,depends_tex_id);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA,size,size,DEPTH,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	
	delete [] data;
}

/*
 */
void GLAppMain::render_particle(const vec3 &pos,float radius,float angle) {
	
	mat4 tmodelview = transpose(modelview);
	vec3 x = tmodelview * vec3(radius,0,0);
	vec3 y = tmodelview * vec3(0,radius,0);
	
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	
	glMatrixMode(GL_TEXTURE);
	glTranslatef(0.5,0.5,0.0);
	glRotatef(angle,0,0,1);
	glTranslatef(-0.5,-0.5,0.0);
	
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0,1);
		glVertex3fv(-x - y);
		glTexCoord2f(1,1);
		glVertex3fv(x - y);
		glTexCoord2f(0,0);
		glVertex3fv(-x + y);
		glTexCoord2f(1,0);
		glVertex3fv(x + y);
	glEnd();
	
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

/*
 */
void GLAppMain::render_flare(const vec3 &pos,const vec3 &color,float radius) {
	
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	flare_tex->enable();
	flare_tex->bind();
	
	glColor3fv(color * 0.4f);
	render_particle(pos,radius * 1.4f,time * dot(color,vec3(1,1,1)) * 16.0f);
	render_particle(pos,radius * 1.2f,-time * dot(color,vec3(1,1,1)) * 16.0f);
	
	glColor3fv(color * 0.3f + vec3(0.0,0.0,0.1));
	render_particle(pos,radius * 0.8f,-time * dot(color,vec3(1,1,1)) * 8.0f);
	
	glColor3fv(color * 0.3f + vec3(0.0,0.1,0.0));
	render_particle(pos,radius * 0.8f,time * dot(color,vec3(1,1,1)) * 8.0f);
	
	glColor3f(1,1,1);
	
	flare_tex->disable();
	
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

/*
 */
void GLAppMain::render_light() {
	
	light_rt->setTarget(light_tex);
	light_rt->enable();
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
		
		light_shader->enable();
		light_shader->bind();
		light_shader->setParameter("light_pos:3",light_pos);
		light_shader->setParameter("light_color:3",light_color);
		
		mesh->render();
		
		light_shader->disable();
	light_rt->flush();
	light_rt->disable();
}

/*
 */
void GLAppMain::render_scattering() {
	
	light_rt->setTarget(light_tex);
	light_rt->enable();
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();		
		
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
		
		scattering_shader->enable();
		scattering_shader->bind();
		
		// light texture
		light_tex->bind(0);
		
		// lockup texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D,depends_tex_id);
		glActiveTexture(GL_TEXTURE0);
		
		mesh->render();
		
		scattering_shader->disable();
	light_rt->flush();
	light_rt->disable();
}

/*
 */
void GLAppMain::render_extrude() {
	
	light_rt->setTarget(light_tex);
	light_rt->enable();
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1,1,-1,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
		
		extrude_shader->enable();
		extrude_shader->bind();
		
		light_tex->bind(0);
		light_tex->render();
		
		extrude_shader->disable();
	light_rt->flush();
	light_rt->disable();
}

/*
 */
void GLAppMain::render() {
	
	// render diffuse to lightmap
	render_light();
	
	// calculate scattering
	if(scattering) {
		render_scattering();
	}
	
	// extrude lightmap contour
	render_extrude();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// matrixes
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	// final render
	final_shader->enable();
	final_shader->bind();
	final_shader->setParameter("light_pos:3",light_pos);
	final_shader->setParameter("camera_pos:3",pos);
	final_shader->setParameter("light_color:3",light_color);
	
	light_tex->bind();
	mesh->render();
	
	final_shader->disable();
	
	// flares
	render_flare(light_pos,light_color,1);
	
	// info
	font->enable(1024,768);
	font->printf(10,10,22,BROWN"FPS:"DEFAULT" %.1f",fps);
	font->printf(10,32,16,"%s\n%s",gl_info,gl_version);
	font->printf(10,64,16,"scattering %s [s]",scattering ? "on" : "off");
	font->disable();
	
	error();
}

/*
 */
void GLAppMain::idle() {
	
	if(keys[KEY_ESC]) exit();
	
	if(keys[KEY_ALT] && keys[KEY_RETURN]) {
		setVideoMode(window_width,window_height,flags ^ FULLSCREEN);
		keys[KEY_ALT] = 0;
		keys[KEY_RETURN] = 0;
	}
	
	if(keys[(int)' ']) {
		pause = !pause;
		keys[(int)' '] = 0;
	}
	
	if(keys[(int)'s']) {
		scattering = !scattering;
		keys[(int)'s'] = 0;
	}
	
	if(pause == 0) {
		time += ifps;
	}
	
	light_pos = vec3(cos(-time / 3.0f) * 7.0f,sin(-time / 3.0f) * 7.0f,sin(time / 13.0f) * 2.0f);
	
	float dx = mouse_x - old_mouse_x;
	float dy = mouse_y - old_mouse_y;
	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;
	
	vec3 z = pos - dir;
	vec3 x = normalize(cross(z,up));
	vec3 y = normalize(cross(z,x));
	if(mouse_button & BUTTON_LEFT) {
		float dist = length(z);
		z = rotate(x,dy * 0.2f) * rotate(y,dx * 0.2f) * z;
		pos = dir + normalize(z) * dist;
		up = normalize(cross(x,z));
	}
	if(mouse_button & BUTTON_RIGHT) {
		float dist = length(z);
		dist += dy * 0.04f;
		if(dist < 2.2f) dist = 2.2f;
		pos = dir + normalize(z) * dist;
	}
	
	projection = perspective(90.0f,(float)window_width / (float)window_height,0.01f,100.0f);
	modelview = look_at(pos,dir,up);
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;
	
	int width = 1024;
	int height = 768;
	int flags = GLApp::MULTISAMPLE_4;
	if(glApp->setVideoMode(width,height,flags) == 0) {
		glApp->exit("can't set %dx%d video mode\n",width,height);
		return 0;
	}
	
	glApp->setTitle("Subsurface scattering http://frustum.org");
	glApp->init();
	glApp->main();
	
	delete glApp;
	
	return 0;
}
