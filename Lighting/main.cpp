/* Lighting demo
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
#include <string.h>
#include "glapp.h"
#include "glext.h"
#include "font.h"
#include "texture.h"
#include "mathlib.h"
#include "mesh.h"
#include "particles.h"
#include "shader.h"
#include "spline.h"

/*
 */
class GLAppMain : public GLApp {
public:
	
	int init();
	void idle();
	
	void render_particle(const vec3 &p,float radius,float angle);
	void render_flare(const vec3 &light,float radius,float angle,const vec3 &color);
	
	int render_get_scissor(const vec3 &light,float radius,int *scissor);
	int render_shadow_volume(const vec3 &light,float radius);
	void render_light(const vec3 &light,float radius,const vec3 &color);
	
	void render_scene();
	
	void render();
	
	int have_ext_stencil_two_side;
	int have_nv_fragment_program;
	int have_nv_depth_clamp;
	
	int show_shadow_volumes_toggle;
	int old_shadow_volumes_toggle;
	int without_cliping_toggle;
	
	int pause;
	float time;
	
	float phi,psi;
	vec3 camera,dir,speed;
	mat4 modelview;
	mat4 projection;
	
	Font *font;
	
	// scene
	Mesh *mesh;
	
	std::vector<int> bricks_surfaces;
	std::vector<int> stone_surfaces;
	
	// lights
	vec3 lights[9];
	vec3 colors[9];
	
	Spline *splines[5];
	
	// particles
	Particles *particles[4];
	
	// shaders
	Shader *ambient_shader;
	Shader *light_shader;
	Shader *shadow_volume_shader;
	Shader *particles_shader;
	
	// textures
	Texture *flare_tex;
	Texture *fire_tex;
	
	Texture *bricks_bs_tex;
	Texture *bricks_n_tex;
	
	Texture *stone_bs_tex;
	Texture *stone_n_tex;
	
	Texture *normalize_tex;
};

/*
 */
int GLAppMain::init() {
	
	glext_init();
	
	// check hardware
	checkExtension("GL_ARB_vertex_buffer_object");
	checkExtension("GL_ARB_shader_objects");
	checkExtension("GL_ARB_vertex_shader");
	checkExtension("GL_ARB_fragment_shader");
	
	// renderer abilities
	char *extensions = (char*)glGetString(GL_EXTENSIONS);
	have_ext_stencil_two_side = strstr(extensions,"GL_EXT_stencil_two_side") ? 1 : 0;
	have_nv_fragment_program = strstr(extensions,"GL_NV_fragment_program") ? 1 : 0;
	have_nv_depth_clamp = strstr(extensions,"GL_NV_depth_clamp") ? 1 : 0;
	
	// toggles
	show_shadow_volumes_toggle = 0;
	old_shadow_volumes_toggle = 0;
	without_cliping_toggle = 0;
	
	// variables
	pause = 0;
	time = 0;
	
	psi = 20.0;
	phi = 6.4;
	camera = vec3(-6.8,1.9,2.2);
	speed = vec3(0,0,0);
	
	// opengl
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// font
	font = new Font("data/font.png");
	
	// scene
	mesh = new Mesh("data/mesh.mesh");
	
	bricks_surfaces.push_back(mesh->findSurface("wall"));
	bricks_surfaces.push_back(mesh->findSurface("column0"));
	bricks_surfaces.push_back(mesh->findSurface("column1"));
	bricks_surfaces.push_back(mesh->findSurface("column2"));
	bricks_surfaces.push_back(mesh->findSurface("column3"));
	
	stone_surfaces.push_back(mesh->findSurface("floor"));
	stone_surfaces.push_back(mesh->findSurface("ceiling"));
	stone_surfaces.push_back(mesh->findSurface("stair"));
	stone_surfaces.push_back(mesh->findSurface("portal"));
	stone_surfaces.push_back(mesh->findSurface("sphere"));
	stone_surfaces.push_back(mesh->findSurface("torch0"));
	stone_surfaces.push_back(mesh->findSurface("torch1"));
	stone_surfaces.push_back(mesh->findSurface("torch2"));
	stone_surfaces.push_back(mesh->findSurface("torch3"));
	
	// paths
	splines[0] = new Spline("data/path_0.txt");
	splines[1] = new Spline("data/path_1.txt");
	splines[2] = new Spline("data/path_2.txt");
	splines[3] = new Spline("data/path_3.txt");
	splines[4] = new Spline("data/path_4.txt");
	
	// particle systems
	particles[0] = new Particles(128,0.2,0.1,0.4,2);
	particles[0]->setForce(vec3(0,0,0.7));
	particles[1] = new Particles(128,0.2,0.1,0.4,2);
	particles[1]->setForce(vec3(0,0,0.7));
	particles[2] = new Particles(128,0.2,0.1,0.4,2);
	particles[2]->setForce(vec3(0,0,0.7));
	particles[3] = new Particles(128,0.2,0.1,0.4,2);
	particles[3]->setForce(vec3(0,0,0.7));
	
	// shaders
	ambient_shader = new Shader("data/shaders/ambient.shader");
	ambient_shader->bindNames("camera:3",NULL);
	
	if(have_nv_fragment_program) light_shader = new Shader("data/shaders/light.shader",NULL,"fragment_nv");
	else light_shader = new Shader("data/shaders/light.shader");
	light_shader->bindNames("light:4","clip_radius:1","camera:3","color:3",NULL);
	
	if(have_nv_depth_clamp) shadow_volume_shader = new Shader("data/shaders/shadow_volume.shader");
	else shadow_volume_shader = new Shader("data/shaders/shadow_volume.shader",NULL,"fragment_depth_clamp");
	shadow_volume_shader->bindNames("light:4",NULL);
	
	particles_shader = new Shader("data/shaders/particles.shader");
	particles_shader->bindNames("tmodelview:9","color:3",NULL);
	
	// textures
	flare_tex = new Texture("data/textures/flare.png",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_NEAREST | Texture::CLAMP | Texture::DXT_3);
	fire_tex = new Texture("data/textures/fire.png",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_NEAREST | Texture::CLAMP | Texture::DXT_3);
	
	bricks_bs_tex = new Texture("data/textures/bricks_bs.dds",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_LINEAR | Texture::DXT_3);
	bricks_n_tex = new Texture("data/textures/bricks_n.jpg",Texture::TEXTURE_2D,Texture::RGB | Texture::LINEAR_MIPMAP_LINEAR);
	
	stone_bs_tex = new Texture("data/textures/stone_bs.dds",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_LINEAR | Texture::DXT_3);
	stone_n_tex = new Texture("data/textures/stone_n.jpg",Texture::TEXTURE_2D,Texture::RGB | Texture::LINEAR_MIPMAP_LINEAR);
	
	normalize_tex = new Texture("data/textures/cube/normalize_%s.png",Texture::TEXTURE_CUBE,Texture::RGB | Texture::LINEAR | Texture::CLAMP_TO_EDGE);
	
	error();
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* rendering                                                                 */
/*                                                                           */
/*****************************************************************************/

/*
 */
void GLAppMain::render_particle(const vec3 &p,float radius,float angle) {
	mat4 tmodelview = modelview.transpose();
	vec3 x = tmodelview * vec3(radius,0,0);
	vec3 y = tmodelview * vec3(0,radius,0);
	glPushMatrix();
	glTranslatef(p.x,p.y,p.z);
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
void GLAppMain::render_flare(const vec3 &light,float radius,float angle,const vec3 &color) {
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	flare_tex->enable();
	flare_tex->bind();
	glColor3fv(saturate(color * 0.7));
	render_particle(light,radius,angle);
	glColor3fv(saturate(color * 0.5 + vec3(0.1,0.1,0.1)));
	render_particle(light,radius * 0.8,-angle);
	flare_tex->disable();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

/*
 */
int GLAppMain::render_get_scissor(const vec3 &light,float radius,int *scissor) {
	
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT,viewport);
	
	mat4 imv = modelview.inverse();
	vec3 camera = imv * vec3(0,0,0);
	
	if((camera - light).length() < radius) {
		scissor[0] = viewport[0];
		scissor[1] = viewport[1];
		scissor[2] = viewport[2];
		scissor[3] = viewport[3];
		return 1;
	}
	
	mat3 tmv = modelview.transpose();
	mat4 mvp = projection * modelview;
	vec3 dz = normalize(camera - light) * radius;
	vec3 dx = tmv * vec3(1,0,0);
	vec3 dy = normalize(cross(dx,dz)) * radius;
	dx = normalize(cross(dy,dz)) * radius;
	
	vec4 p[4];
	p[0] = mvp * vec4(light - dx + dz,1);
	p[1] = mvp * vec4(light + dx + dz,1);
	p[2] = mvp * vec4(light - dy + dz,1);
	p[3] = mvp * vec4(light + dy + dz,1);
	
	if(p[0].z < 0.0 || p[1].z < 0.0 || p[2].z < 0.0 || p[3].z < 0.0) {
		
		p[0] = mvp * vec4(light - dx - dz,1);
		p[1] = mvp * vec4(light + dx - dz,1);
		p[2] = mvp * vec4(light - dy - dz,1);
		p[3] = mvp * vec4(light + dy - dz,1);
		
		if(p[0].z < 0.0 && p[1].z < 0.0 && p[2].z < 0.0 && p[3].z < 0.0) {
			scissor[0] = 0;
			scissor[1] = 0;
			scissor[2] = 0;
			scissor[3] = 0;
			return 0;
		}
		
		scissor[0] = viewport[0];
		scissor[1] = viewport[1];
		scissor[2] = viewport[2];
		scissor[3] = viewport[3];
		return 1;
	}
	
	p[0] /= p[0].w;
	p[1] /= p[1].w;
	p[2] /= p[2].w;
	p[3] /= p[3].w;
	
	scissor[0] = viewport[0] + (int)((float)viewport[2] * (p[0].x + 1.0) / 2.0);
	scissor[2] = viewport[0] + (int)((float)viewport[2] * (p[1].x + 1.0) / 2.0);	
	scissor[1] = viewport[1] + (int)((float)viewport[3] * (p[2].y + 1.0) / 2.0);
	scissor[3] = viewport[1] + (int)((float)viewport[3] * (p[3].y + 1.0) / 2.0);
	
	if(scissor[0] > scissor[2]) { int i = scissor[0]; scissor[0] = scissor[2]; scissor[2] = i; }
	if(scissor[1] > scissor[3]) { int i = scissor[1]; scissor[1] = scissor[3]; scissor[3] = i; }
	
	if(scissor[0] < viewport[0]) scissor[0] = viewport[0];
	else if(scissor[0] > viewport[0] + viewport[2]) scissor[0] = viewport[0] + viewport[2];
	if(scissor[2] < viewport[0]) scissor[2] = viewport[0];
	else if(scissor[2] > viewport[2] + viewport[3]) scissor[2] = viewport[0] + viewport[2];

	if(scissor[1] < viewport[1]) scissor[1] = viewport[1];
	else if(scissor[1] > viewport[1] + viewport[3]) scissor[1] = viewport[1] + viewport[3];
	if(scissor[3] < viewport[1]) scissor[3] = viewport[1];
	else if(scissor[3] > viewport[1] + viewport[3]) scissor[3] = viewport[1] + viewport[3];
	
	scissor[2] -= scissor[0];
	scissor[3] -= scissor[1];
	
	return 1;
}

/*
 */
int GLAppMain::render_shadow_volume(const vec3 &light,float radius) {
	
	int ret = 0;
	
	if(show_shadow_volumes_toggle) glEnable(GL_BLEND);
	else glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	
	if(have_nv_depth_clamp) glEnable(GL_DEPTH_CLAMP_NV);
	
	if(have_ext_stencil_two_side) {
		glDisable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		
		glActiveStencilFaceEXT(GL_BACK);
		glStencilFunc(GL_ALWAYS,0,~0);
		glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
		glActiveStencilFaceEXT(GL_FRONT);
		glStencilFunc(GL_ALWAYS,0,~0);
		glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
	}
	
	shadow_volume_shader->enable();
	shadow_volume_shader->bind();
	for(int i = 0; i < mesh->getNumSurfaces(); i++) {
		
		float shadow_radius = old_shadow_volumes_toggle ? 100000.0 : radius;
		
		if(mesh->findSilhouette(light,shadow_radius,i) == 0) continue;
		int n = mesh->getNumIntersections(light,shadow_radius,camera,i);
		
		if(n == 0) shadow_volume_shader->setParams((float*)vec4(light,shadow_radius));
		else shadow_volume_shader->setParams((float*)vec4(light,100000.0));
		
		if(have_ext_stencil_two_side) {
			mesh->renderShadowVolume(i);
		} else {
			glCullFace(GL_FRONT);
			glStencilFunc(GL_ALWAYS,0,~0);
			glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
			mesh->renderShadowVolume(i);
			
			glCullFace(GL_BACK);
			glStencilFunc(GL_ALWAYS,0,~0);
			glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
			mesh->renderShadowVolume(i);
		}
		
		ret += n;
	}
	shadow_volume_shader->disable();
	
	if(have_ext_stencil_two_side) {
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glEnable(GL_CULL_FACE);
	}
	
	if(have_nv_depth_clamp) glDisable(GL_DEPTH_CLAMP_NV);
	
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

	return ret;
}

/*
 */
void GLAppMain::render_light(const vec3 &light,float radius,const vec3 &color) {
		
	int scissor[4];
	if(render_get_scissor(light,radius,scissor) == 0) return;
	
	glEnable(GL_SCISSOR_TEST);
	glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
	
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);
	
	glClear(GL_STENCIL_BUFFER_BIT);
	
	int val = render_shadow_volume(light,radius * 1.2);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	glStencilFunc(GL_EQUAL,val,~0);
	
	light_shader->enable();
	
	float clip_radius = without_cliping_toggle ? 100000.0 : radius;
	light_shader->bind((float*)vec4(light,radius),&clip_radius,(float*)camera,(const float*)color);
	
	normalize_tex->bind(2);
	
	bricks_bs_tex->bind(0);
	bricks_n_tex->bind(1);
	for(int i = 0; i < (int)bricks_surfaces.size(); i++) {
		if(bricks_surfaces[i] == -1) continue;
		mesh->render(light,radius,bricks_surfaces[i]);
	}
	
	stone_bs_tex->bind(0);
	stone_n_tex->bind(1);
	for(int i = 0; i < (int)stone_surfaces.size(); i++) {
		if(stone_surfaces[i] == -1) continue;
		mesh->render(light,radius,stone_surfaces[i]);
	}
	
	light_shader->disable();
	
	glDisable(GL_BLEND);
	
	glDisable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	
	glDisable(GL_SCISSOR_TEST);
}

/*
 */
void GLAppMain::render_scene() {
	
	glClear(GL_DEPTH_BUFFER_BIT);
	
	ambient_shader->enable();
	ambient_shader->bind((float*)camera);
	mesh->render();
	ambient_shader->disable();
	
	render_light(lights[0],6,colors[0]);
	
	render_light(lights[1],4.5,colors[1]);
	render_light(lights[2],4.5,colors[2]);
	render_light(lights[3],4.5,colors[3]);
	render_light(lights[4],4.5,colors[4]);
	
	render_light(lights[5],2.5,colors[5]);
	render_light(lights[6],2.5,colors[6]);
	render_light(lights[7],2.5,colors[7]);
	render_light(lights[8],2.5,colors[8]);
	
	render_flare(lights[0],0.4,time * 60,colors[0]);
	
	render_flare(lights[1],0.35,time * 60,colors[0]);
	render_flare(lights[2],0.35,time * 70,colors[1]);
	render_flare(lights[3],0.35,-time * 50,colors[2]);
	render_flare(lights[4],0.35,-time * 65,colors[3]);
	
	particles_shader->enable();
	mat3 tmodelview = modelview.transpose();
	particles_shader->bind();
	
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	fire_tex->enable();
	fire_tex->bind();
	
	particles_shader->setParams((float*)tmodelview,(float*)colors[5]);
	particles[0]->render();
	
	particles_shader->setParams((float*)tmodelview,(float*)colors[6]);
	particles[1]->render();
	
	particles_shader->setParams((float*)tmodelview,(float*)colors[7]);
	particles[2]->render();
	
	particles_shader->setParams((float*)tmodelview,(float*)colors[8]);
	particles[3]->render();
	
	fire_tex->disable();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	
	particles_shader->disable();
}

/*
 */
void GLAppMain::render() {
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	render_scene();
	
	error();
	
	// info
	font->enable(1600,1200);
	glColor3fv(vec3(1,1,0.2));
	font->printf(5,5,"fps: %.0f",fps);
	font->disable();
	
	font->enable(2048,1536);
	glColor3fv(vec3(1,1,0.2) * 0.8);
	font->printf(8,50,"%s\n%s\nshow shadow volumes %s [1]\nold shadow volumes %s [2]\nwithout cliping %s [3]",
		glGetString(GL_RENDERER),glGetString(GL_VERSION),
		show_shadow_volumes_toggle ? "on" : "off",
		old_shadow_volumes_toggle ? "on" : "off",
		without_cliping_toggle ? "yes" : "no");
	font->disable();
}

/*
 */
void GLAppMain::idle() {
	
	if(!pause) time += ifps;
	
	lights[0] = (*splines[0])(time,1);
	colors[0] = vec3(1.0,0.9,0.8);
	
	lights[1] = (*splines[1])(time,1);
	colors[1] = vec3(1.0,0.8,0.5);
	
	lights[2] = (*splines[2])(time,1);
	colors[2] = vec3(1.0,0.9,0.8);
	
	lights[3] = (*splines[3])(time,1);
	colors[3] = vec3(1.0,0.7,0.4);
	
	lights[4] = (*splines[4])(time,1);
	colors[4] = vec3(1.0,0.7,0.3);
	
	float k0 = fabs(sin(time * 4) * 0.5 + sin(time * 6) * 0.6 + cos(time * 7) * 0.5 + sin(time * 13) * 0.25);
	float k1 = sin(time * 13) * 0.1 + cos(time * 15) * 0.1 + sin(time * 12) * 0.1 + cos(time * 3) * 0.1;
	lights[5] = vec3(3.2 + sin(time) * 0.1 * k1,3.3 + cos(time) * 0.1 * k1,4.1);
	colors[5] = saturate((vec3(0.929,0.653,0.184) * k0 + vec3(0.803,0.435,0.058) * (1.0 - k0)) * (1.0 - k1));
	
	k0 = fabs(sin(time * 5) * 0.5 + sin(time * 4) * 0.6 + cos(time * 6) * 0.5 + sin(time * 11) * 0.25);
	k1 = sin(time * 9) * 0.1 + cos(time * 13) * 0.1 + sin(time * 15) * 0.1 + cos(time * 2.5) * 0.2;
	lights[6] = vec3(3.3 + sin(time) * 0.1 * k1,-3.2 + cos(time) * 0.1 * k1,4.1);
	colors[6] = saturate((vec3(0.929,0.653,0.184) * k0 + vec3(0.803,0.435,0.058) * (1.0 - k0)) * (1.0 - k1));
	
	k0 = fabs(sin(time * 6) * 0.5 + sin(time * 7) * 0.6 + cos(time * 4) * 0.5 + sin(time * 12) * 0.25);
	k1 = sin(time * 12) * 0.1 + cos(time * 9) * 0.1 + sin(time * 11) * 0.1 + cos(time * 2.0) * 0.2;
	lights[7] = vec3(-3.4 + sin(time) * 0.1 * k1,3.3 + cos(time) * 0.1 * k1,4.1);
	colors[7] = saturate((vec3(0.929,0.653,0.184) * k0 + vec3(0.803,0.435,0.058) * (1.0 - k0)) * (1.0 - k1));
	
	k0 = fabs(sin(time * 3.6) * 0.5 + sin(time * 5) * 0.6 + cos(time * 5) * 0.5 + sin(time * 9) * 0.25);
	k1 = sin(time * 11) * 0.1 + cos(time * 13) * 0.1 + sin(time * 9) * 0.1 + cos(time * 2.7) * 0.2;
	lights[8] = vec3(-3.3 + sin(time) * 0.1 * k1,-3.3 + cos(time) * 0.1 * k1,4.1);
	colors[8] = saturate((vec3(0.929,0.653,0.184) * k0 + vec3(0.803,0.435,0.058) * (1.0 - k0)) * (1.0 - k1));
	
	// update particles
	particles[0]->set(lights[5] + vec3(0,0,-0.3));
	particles[0]->setColor(colors[5]);
	particles[0]->update(ifps);
	
	particles[1]->set(lights[6] + vec3(0,0,-0.3));
	particles[1]->setColor(colors[6]);
	particles[1]->update(ifps);
	
	particles[2]->set(lights[7] + vec3(0,0,-0.3));
	particles[2]->setColor(colors[7]);
	particles[2]->update(ifps);
	
	particles[3]->set(lights[8] + vec3(0,0,-0.3));
	particles[3]->setColor(colors[8]);
	particles[3]->update(ifps);
	
	// keyboard events
	if(keys[KEY_ESC]) exit();
	
	if(keys[(int)' ']) {
		pause = !pause;
		keys[(int)' '] = 0;
	}
	
	if(keys[(int)'1']) {
		show_shadow_volumes_toggle = !show_shadow_volumes_toggle;
		keys[(int)'1'] = 0;
	}
	
	if(keys[(int)'2']) {
		old_shadow_volumes_toggle = !old_shadow_volumes_toggle;
		keys[(int)'2'] = 0;
	}
	
	if(keys[(int)'3']) {
		without_cliping_toggle = !without_cliping_toggle;
		keys[(int)'3'] = 0;
	}
	
	static int look = 0;
	
	if(!look && mouseButton & BUTTON_LEFT) {
		setCursor(windowWidth / 2,windowHeight / 2);
		mouseX = windowWidth / 2;
		mouseY = windowHeight / 2;
		look = 1;
	}
	if(mouseButton & BUTTON_RIGHT) look = 0;
	
	if(look) {
		showCursor(0);
		psi += (mouseX - windowWidth / 2) * 0.2;
		phi += (mouseY - windowHeight / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 89) phi = 89;
		setCursor(windowWidth / 2,windowHeight / 2);
	} else showCursor(1);
	
	float vel = 15;
	if(keys[KEY_UP] || keys[(int)'w']) speed.x += vel * ifps;
	if(keys[KEY_DOWN] || keys[(int)'s']) speed.x -= vel * ifps;
	if(keys[KEY_LEFT] || keys[(int)'a']) speed.y -= vel * ifps;
	if(keys[KEY_RIGHT] || keys[(int)'d']) speed.y += vel * ifps;
	if(keys[KEY_SHIFT]) speed.z += vel * ifps;
	if(keys[KEY_CTRL]) speed.z -= vel * ifps;
	
	speed -= speed * 5 * ifps;
	
	quat q0,q1;
	q0.set(vec3(0,0,1),-psi);
	q1.set(vec3(0,1,0),phi);
	dir = (q0 * q1).to_matrix() * vec3(1,0,0);
	vec3 x,y,z;
	x = dir;
	y.cross(dir,vec3(0,0,1));
	y.normalize();
	z.cross(y,x);
	
	vec3 old = camera;
	camera += (x * speed.x + y * speed.y + z * speed.z) * ifps;
	
	// simplest scene sliding
	for(int i = 0; i < 8; i++) {
		vec3 v;
		if(i == 0) v = vec3(1,1,-1);
		else if(i == 1) v = vec3(1,-1,-1);
		else if(i == 2) v = vec3(-1,1,-1);
		else if(i == 3) v = vec3(-1,-1,-1);
		else if(i == 4) v = vec3(1,1,1);
		else if(i == 5) v = vec3(1,-1,1);
		else if(i == 6) v = vec3(-1,1,1);
		else if(i == 7) v = vec3(-1,-1,1);
		v *= 0.3;
		vec3 p,n;
		if(mesh->intersection(old + v,camera + v,p,n)) {
			float d = (camera - old) * n;
			if(d < 0) camera -= n * d;
		}
	}
	
	modelview.look_at(camera,camera + dir,vec3(0,0,1));
	
	projection.perspective(90.0,(float)windowWidth / (float)windowHeight,0.1,1000);
}

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;
	
	int width = 1024;
	int height = 768;
	int flags = 0;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i],"-w") && i < argc - 1) sscanf(argv[++i],"%d",&width);
		else if(!strcmp(argv[i],"-h") && i < argc - 1) sscanf(argv[++i],"%d",&height);
		else if(!strcmp(argv[i],"-fs")) flags |= GLApp::FULLSCREEN;
		else if(!strcmp(argv[i],"-2x")) flags |= GLApp::MULTISAMPLE_2;
		else if(!strcmp(argv[i],"-4x")) flags |= GLApp::MULTISAMPLE_4;
		else {
			glApp->exit("unknown option \"%s\"",argv[i]);
			break;
		}
	}

	if(!glApp->done) {
		
		if(!glApp->setVideoMode(width,height,flags)) return 0;
		
		glApp->setTitle("Lighting demo http://frustum.org");
		
		if(!glApp->init()) glApp->exit("initialization failed\n");
		
		glApp->main();
	}
	
	delete glApp;
	
	return 0;
}
