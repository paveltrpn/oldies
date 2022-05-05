/*	water demo
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "mathlib.h"
#include "mesh.h"
#include "font.h"
#include "object.h"

#define WIDTH	1024
#define HEIGHT	768

int my_pause = 1;
float time_,fps,phi = -25,psi = -40,dist = 500;
vec3 camera;
mat4 modelview;

Font *font;

Object *object;
Mesh *plane;
Mesh *box;
Mesh *water;
Mesh *sky0,*sky1;
Texture *sky0_t,*sky1_t;
Texture *metall;
Texture *plane_texture;
Texture *vrtainment;

float car_height = 20;

vec4 fog_color = vec4(26.0 / 255.0,66.0 / 255.0,123.0 / 255.0,1);

#define REFLECT_SIZE 256
GLuint reflect_texture;
#define REFRACT_SIZE 256
GLuint refract_texture;
#define OFFSET_SIZE 256
GLuint offset_texture;

enum {
	KEY_A,
	KEY_Z
};

int key_mask[256];

/*
 */
void create_offset_texture(int width,int height,unsigned char *data) {
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float fx = x / (float)width - 0.5;
			float fy = y / (float)height - 0.5;
			float r = sqrt(fx*fx + fy*fy);

			unsigned char ds = (unsigned char)(64 * cos(300.0 * r) * exp(-r * 5.0));
			ds += (unsigned char)(32 * cos(150.0 * (fx + fy)));
			ds += (unsigned char)(16 * cos(140.0 * (fx * 0.85 - fy)));
			
			unsigned char dt = (unsigned char)(64 * sin(300.0 * r) * exp(-r * 5.0));
			dt += (unsigned char)(32 * sin(150.0 * (fx + fy)));
			dt += (unsigned char)(16 * sin(140.0 * (fx * 0.85 - fy)));
			
			*data++ = ds;
			*data++ = dt;
		}
	}
}

/*
 */
int init(void) {
	
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	
	font = new Font("data/font.tga");
	
	object = new Object("data/audi/object");
	
	plane = new Mesh("data/plane.3ds");
	water = new Mesh("data/water.3ds");
	box = new Mesh("data/box.3ds");
	sky0 = new Mesh("data/sky_0.3ds");
	sky1 = new Mesh("data/sky_1.3ds");
	
	plane_texture = new Texture("data/plane.jpg");
	metall = new Texture("data/metall.jpg");
	sky0_t = new Texture("data/sky_0.jpg",Texture::TRILINEAR | Texture::MIPMAP_SGIS | Texture::CLAMP);
	sky1_t = new Texture("data/sky_1.jpg");
	vrtainment = new Texture("data/vrtainment.jpg");
	
	glGenTextures(1,&reflect_texture);
	glBindTexture(GL_TEXTURE_2D,reflect_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,REFLECT_SIZE,REFLECT_SIZE,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
	
	glGenTextures(1,&refract_texture);
	glBindTexture(GL_TEXTURE_2D,refract_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,REFRACT_SIZE,REFRACT_SIZE,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
	
	glGenTextures(1,&offset_texture);
	glBindTexture(GL_TEXTURE_2D,offset_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	unsigned char *offset_data = new unsigned char[OFFSET_SIZE * OFFSET_SIZE * 2];
	create_offset_texture(OFFSET_SIZE,OFFSET_SIZE,offset_data);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DSDT_NV,OFFSET_SIZE,OFFSET_SIZE,0,GL_DSDT_NV,GL_UNSIGNED_BYTE,offset_data);
	
	return 0;
}

/* get fps
 */
float get_fps(void) {
	static float fps = 60;
	static int starttime,endtime,counter;
	if(counter == 10) {
		endtime = starttime;
		starttime = SDL_GetTicks();
		fps = counter * 1000.0 / (float)(starttime - endtime);
		counter = 0;
	}
	counter++;
	return fps;
}

/*
 */
void render_scene() {
	
	glPushMatrix();
	glScalef(8000,8000,8000);
	glDepthMask(GL_FALSE);
	
	sky0_t->enable();
	sky0_t->bind();
	sky0->render();
	sky0_t->disable();
	
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glFogi(GL_FOG_DISTANCE_MODE_NV,GL_EYE_RADIAL_NV);
	glFogf(GL_FOG_START,4000);
	glFogf(GL_FOG_END,4000 * 1.9);
	glFogfv(GL_FOG_COLOR,fog_color.v);
	sky1_t->enable();
	sky1_t->bind();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(time_ / 40,0,0);
	glScalef(2,2,2);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	sky1->render();
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	sky1_t->disable();
	glDisable(GL_FOG);
	
	glDepthMask(GL_TRUE);
	glPopMatrix();
	
	plane_texture->enable();
	plane_texture->bind();
	plane->render();
	plane_texture->disable();
	glPushMatrix();
	glTranslatef(0,0,car_height);
	object->render();
	metall->enable();
	metall->bind();
	box->render();
	metall->disable();
	glPopMatrix();
}

/*
 */
void render_water_texture() {
	// reflect texture
	glViewport(0,0,REFLECT_SIZE,REFLECT_SIZE);
	glScissor(0,0,REFLECT_SIZE,REFLECT_SIZE);
	glEnable(GL_SCISSOR_TEST);
	
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,4.0 / 3.0,1,10000);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	glScalef(1,1,-1);
	
	glDisable(GL_CULL_FACE);
	glEnable(GL_CLIP_PLANE0);
	double reflect_clip[] = { 0, 0, 1, 0 };
	glClipPlane(GL_CLIP_PLANE0,reflect_clip);
	render_scene();
	glDisable(GL_CLIP_PLANE0);
	glEnable(GL_CULL_FACE);
	
	glBindTexture(GL_TEXTURE_2D,reflect_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,REFLECT_SIZE,REFLECT_SIZE);
	
	// refract texture
	glViewport(0,0,REFRACT_SIZE,REFRACT_SIZE);
	glScissor(0,0,REFRACT_SIZE,REFRACT_SIZE);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,4.0 / 3.0,1,10000);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	
	glDisable(GL_CULL_FACE);
	glEnable(GL_CLIP_PLANE0);
	double refract_clip[] = { 0, 0, -1, 0 };
	glClipPlane(GL_CLIP_PLANE0,refract_clip);
	render_scene();
	glDisable(GL_CLIP_PLANE0);
	glEnable(GL_CULL_FACE);
	
	glBindTexture(GL_TEXTURE_2D,refract_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,REFRACT_SIZE,REFRACT_SIZE);
	
	glViewport(0,0,WIDTH,HEIGHT);
	glDisable(GL_SCISSOR_TEST);
}

/*
 */
void render_water() {
	float k = time_;
	float plane_s[] = { 1, 0, 0, 0 };
	float plane_t[] = { 0, 1, 0, 0 };
	float plane_r[] = { 0, 0, 1, 0 };
	float plane_q[] = { 0, 0, 0, 1 };
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_SHADER_NV,GL_SHADER_OPERATION_NV,GL_TEXTURE_2D);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glTexEnvi(GL_TEXTURE_SHADER_NV,GL_SHADER_OPERATION_NV,GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV);
	glTexEnvi(GL_TEXTURE_SHADER_NV,GL_PREVIOUS_TEXTURE_INPUT_NV,GL_TEXTURE0_ARB);
	glTexEnvfv(GL_TEXTURE_SHADER_NV,GL_OFFSET_TEXTURE_2D_MATRIX_NV,(vec4(cos(k),-sin(k),sin(k),cos(k)) * 0.02).v);
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glTexEnvi(GL_TEXTURE_SHADER_NV,GL_SHADER_OPERATION_NV,GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV);
	glTexEnvi(GL_TEXTURE_SHADER_NV,GL_PREVIOUS_TEXTURE_INPUT_NV,GL_TEXTURE0_ARB);
	glTexEnvfv(GL_TEXTURE_SHADER_NV,GL_OFFSET_TEXTURE_2D_MATRIX_NV,(vec4(cos(k),-sin(k),sin(k),cos(k)) * 0.01).v);
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV,vec4(1,1,1,1).v);
	glCombinerParameterfvNV(GL_CONSTANT_COLOR1_NV,vec4(0.3,0.3,0.4,1).v);
	
	glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV,1);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_A_NV,GL_TEXTURE1_ARB,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_B_NV,GL_CONSTANT_COLOR0_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_C_NV,GL_TEXTURE2_ARB,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_D_NV,GL_CONSTANT_COLOR1_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerOutputNV(GL_COMBINER0_NV,GL_RGB,GL_DISCARD_NV,GL_DISCARD_NV,GL_SPARE0_NV,GL_NONE,GL_NONE,GL_FALSE,GL_FALSE,GL_FALSE);

	glFinalCombinerInputNV(GL_VARIABLE_A_NV,GL_SPARE0_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_B_NV,GL_ZERO,GL_UNSIGNED_INVERT_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_C_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_D_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_E_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_F_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	
	glEnable(GL_TEXTURE_SHADER_NV);
	glEnable(GL_REGISTER_COMBINERS_NV);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D,reflect_texture);
	glTexGenfv(GL_S,GL_EYE_PLANE,plane_s);
	glTexGenfv(GL_T,GL_EYE_PLANE,plane_t);
	glTexGenfv(GL_R,GL_EYE_PLANE,plane_r);
	glTexGenfv(GL_Q,GL_EYE_PLANE,plane_q);
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.5,0.5,0);
	glScalef(0.5,0.5,0);
	gluPerspective(45,4.0 / 3.0,1,10000);
	glMultMatrixf(modelview.mat);
	glScalef(1,1,-1);
	glMatrixMode(GL_MODELVIEW);
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glBindTexture(GL_TEXTURE_2D,refract_texture);
	glTexGenfv(GL_S,GL_EYE_PLANE,plane_s);
	glTexGenfv(GL_T,GL_EYE_PLANE,plane_t);
	glTexGenfv(GL_R,GL_EYE_PLANE,plane_r);
	glTexGenfv(GL_Q,GL_EYE_PLANE,plane_q);
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.5,0.5,0);
	glScalef(0.5,0.5,0);
	gluPerspective(45,4.0 / 3.0,1,10000);
	glMultMatrixf(modelview.mat);
	glMatrixMode(GL_MODELVIEW);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glBindTexture(GL_TEXTURE_2D,offset_texture);
	
	water->render();
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glDisable(GL_REGISTER_COMBINERS_NV);
	glDisable(GL_TEXTURE_SHADER_NV);
}

/*
 */
void render() {
	
	render_water_texture();
	
	glClearColor(fog_color[0],fog_color[1],fog_color[2],fog_color[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45,4.0 / 3.0,1,10000);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	
	render_water();
	
	render_scene();
	
	font->printf(640,480,10,10,"fps: %.0f",fps);
	glColor3f(0,1,0);
	font->printf(1024,768,15,50,"camera %.0f %.0f %.0f\ncar height %.0f (a/z)\npause: %s (space)\n%s",
		camera.x,camera.y,camera.z,car_height,my_pause ? "yes" : "no",glGetString(GL_RENDERER));
	glColor3f(0.55,0.67,0.80);
	font->printf(1024,768,700,10,"The vehicle model provided courtesy of VRtainment\n    Copyright VRtainment www.vrtainment.de");
	glColor3f(1,1,1);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1024,768,0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_SRC_ALPHA);
	glColor4f(1,1,1,0.4);
	vrtainment->enable();
	vrtainment->bind();
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(1,0);
	glVertex2f(910,50);
	glTexCoord2f(0,0);
	glVertex2f(788,50);
	glTexCoord2f(1,1);
	glVertex2f(910,90);
	glTexCoord2f(0,1);
	glVertex2f(788,90);
	glEnd();
	vrtainment->disable();
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
	glDepthFunc(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	SDL_GL_SwapBuffers();
}

/*
 */
void idle(void) {
	quat q0,q1,q2;
	
	fps = get_fps();
	float ifps = 1.0 / fps;
	time_ += ifps;
	
	if(key_mask[KEY_A]) car_height += 100 * ifps;
	if(key_mask[KEY_Z]) car_height -= 100 * ifps;
	if(car_height > 90) car_height = 90;
	if(car_height < -250) car_height = -250;
	
	if(!my_pause) psi += 15 * ifps;
	
	vec3 dir = vec3(0,0,10);
	vec3 up = vec3(0,0,1);
	vec3 v = vec3(0,0,1);
	q0.set(v,psi);
	v = vec3(0,1,0);
	q1.set(v,phi);
	q2 = q0 * q1;
	mat4 m0 = q2.to_matrix();
	camera = m0 * vec3(dist,0,0) + dir;
	modelview.lock_at(camera,dir,up);
}

/*
 */
void keyboard_down(int key) {
	static unsigned int flag;
	switch(key) {
		case SDLK_ESCAPE:
			SDL_Quit();
			exit(0);
			break;
		case SDLK_f:
			if(flag == SDL_FULLSCREEN) flag = 0;
			else flag = SDL_FULLSCREEN;
			SDL_SetVideoMode(WIDTH,HEIGHT,32,SDL_OPENGL | flag);
			break;
		case SDLK_a: key_mask[KEY_A] = 1; break;
		case SDLK_z: key_mask[KEY_Z] = 1; break;
		case SDLK_SPACE: my_pause = !my_pause; break;
	}
}

/*
 */
void keyboard_up(int key) {
	switch(key) {
		case SDLK_a: key_mask[KEY_A] = 0; break;
		case SDLK_z: key_mask[KEY_Z] = 0; break;
	}
}

/*
 */
void mouse(int button,int state,int x,int y) {
	if(button == 0) {
		psi += (x - WIDTH / 2) * 0.2;
		phi += (y - HEIGHT / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 0) phi = 0;
	}
	if(button == 4 && state == SDL_PRESSED) {
		dist -= 20;
		if(dist < 20) dist = 20;
	}
	if(button == 5 && state == SDL_PRESSED) {
		dist += 20;
		if(dist > 1000) dist = 1000;
	}
}

/*
 */
int main(int argc,char **argv) {
	int done;
	
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetVideoMode(WIDTH,HEIGHT,32,SDL_OPENGL);
	
	if(init() != 0) return 1;
	
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WarpMouse(WIDTH / 2,HEIGHT / 2);
	
	done = 0;
	while(!done) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = 1;
					break;
				case SDL_KEYDOWN:
					keyboard_down(event.key.keysym.sym);
					break;
				case SDL_KEYUP:
					keyboard_up(event.key.keysym.sym);
					break;
				case SDL_MOUSEMOTION:
					mouse(event.button.button,event.button.state,event.button.x,event.button.y);
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					mouse(event.button.button,event.button.state,event.button.x,event.button.y);
					break;
			}
		}
		SDL_WarpMouse(WIDTH / 2,HEIGHT / 2);
		idle();
		render();
	}
	
	SDL_Quit();
	return 0;
}
