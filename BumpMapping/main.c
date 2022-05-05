#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "mathlib.h"
#include "texture.h"
#include "bump_mesh.h"
#include "font.h"

#define WIDTH	1024
#define HEIGHT	768

/* frame per second and time */
float fps,time;

int mode;

/* camera parameter */
float phi = -30,psi,dist = 100;
matrix_t modelview;

/* light sources */
vec4_t light;

/* font */
font_t *font;

/* normalized cube map texture */
int cube_map_id;

/* start position */
vec3_t plane_pos = { 0, 0, 0 };
vec3_t mesh0_pos = { 15, 10, 18 };
vec3_t mesh1_pos = { -7, -13, 15 };

/* transform matrixes */
matrix_t plane_matrix;
matrix_t mesh0_matrix;
matrix_t mesh1_matrix;

/* model lists */
bump_mesh_t *plane_bm;
bump_mesh_t *mesh0_bm;
bump_mesh_t *mesh1_bm;

/* textures */
int plane_base;
int plane_dot3;
int mesh0_base;
int mesh0_dot3;
int mesh1_base;
int mesh1_dot3;

/*
 */
int init(void) {
	
	glClearDepth(1);
	glClearColor(0,0,0,0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glPointSize(5.0);
	
	/* load font */
	font = font_load("data/font.tga");
	
	//cube_map_id = bump_create_cube_map(64);
	
	/* load objects */
	plane_bm = bump_mesh_load("data/plane.3ds");
	if(!plane_bm) return 1;
	
	mesh0_bm = bump_mesh_load("data/mesh0.3ds");
	if(!mesh0_bm) return 1;
	
	mesh1_bm = bump_mesh_load("data/mesh1.3ds");
	if(!mesh1_bm) return 1;
	
	m_translate(plane_pos,plane_matrix);
	m_translate(mesh0_pos,mesh0_matrix);
	m_translate(mesh1_pos,mesh1_matrix);
	
	plane_base = texture_load("data/plane.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	plane_dot3 = texture_load("data/plane_dot3.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	
	mesh0_base = texture_load("data/mesh0.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	mesh0_dot3 = texture_load("data/mesh0_dot3.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	
	mesh1_base = texture_load("data/mesh1.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	mesh1_dot3 = texture_load("data/mesh1_dot3.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);

	glEnable(GL_REGISTER_COMBINERS_NV);
	glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV,1);
	
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_A_NV,GL_TEXTURE1_ARB,GL_EXPAND_NORMAL_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_B_NV,GL_PRIMARY_COLOR_NV,GL_EXPAND_NORMAL_NV,GL_RGB);
	glCombinerOutputNV(GL_COMBINER0_NV,GL_RGB,GL_SPARE0_NV,GL_DISCARD_NV,GL_DISCARD_NV,GL_NONE,GL_NONE,GL_TRUE,GL_FALSE,GL_FALSE);
	
	glFinalCombinerInputNV(GL_VARIABLE_A_NV,GL_SPARE0_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_B_NV,GL_TEXTURE0_ARB,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_C_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_D_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_E_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_F_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_G_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_ALPHA);
	
	glDisable(GL_REGISTER_COMBINERS_NV);
	
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
void render_mesh(bump_mesh_t *mesh,float *matrix,int base,int dot3,float *light) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,base);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,dot3);
	
	bump_mesh_render_light(mesh,matrix,light);
	
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
}

/*
 */
void render(void) {
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45,4.0 / 3.0,1,1000);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	glLightfv(GL_LIGHT0,GL_POSITION,light);
	
	glEnable(GL_REGISTER_COMBINERS_NV);

	render_mesh(plane_bm,plane_matrix,plane_base,plane_dot3,light);
	render_mesh(mesh0_bm,mesh0_matrix,mesh0_base,mesh0_dot3,light);
	render_mesh(mesh1_bm,mesh1_matrix,mesh1_base,mesh1_dot3,light);
	
	glDisable(GL_REGISTER_COMBINERS_NV);
	
	glBegin(GL_POINTS);
	glVertex3fv(light);
	glEnd();
	
	font_printf(font,640,480,10,10,"fps %.2f\nbump mapping demo\nтекстуры из Doom III",fps);
	
	SDL_GL_SwapBuffers();
}

/*
 */
void idle(void) {
	float ifps;
	vec3_t v,camera,dir,up;
	vec4_t q0,q1,q2;
	matrix_t m0,m1,m2;
	
	fps = get_fps();
	ifps = 1.0 / fps;
	if(mode != 1) time += ifps;
	
	v_set(sin(time * 360 / 10 * deg2rad) * 40,cos(time * 360 / 10 * deg2rad) * 40,sin(time * 360 / 5 * deg2rad) * 10 + 20,light);
	light[3] = 0;
	
	if(mode != 2) {
		m_rotation_x(ifps * 360 / 20,m0);
		m_rotation_z(ifps * 360 / 20,m1);
		m_multiply(m0,m1,m2);
		m_copy(mesh0_matrix,m0);
		m_multiply(m0,m2,mesh0_matrix);
	
		m_rotation_y(ifps * 360 / 25,m0);
		m_rotation_z(-ifps * 360 / 30,m1);
		m_multiply(m0,m1,m2);
		m_copy(mesh1_matrix,m0);
		m_multiply(m0,m2,mesh1_matrix);
	}
	
	/* create modelview matrix */
	v_set(0,0,0,dir);
	v_set(0,0,1,up);
	v_set(0,0,1,v);
	q_set(v,psi,q0);
	v_set(0,1,0,v);
	q_set(v,phi,q1);
	q_multiply(q0,q1,q2);
	q_to_matrix(q2,m0);
	v_set(dist,0,0,camera);
	v_transform(camera,m0,camera);
	v_add(camera,dir,camera);
	m_look_at(camera,dir,up,modelview);

}

/*
 */
void keyboard(int key) {
	static int flag;
	switch(key) {
		case SDLK_ESCAPE:
			SDL_Quit();
			exit(0);
			break;
		case SDLK_SPACE:
			mode++;
			if(mode == 3) mode = 0;
			break;
		case SDLK_f:
			if(flag == SDL_FULLSCREEN) flag = 0;
			else flag = SDL_FULLSCREEN;
			SDL_SetVideoMode(WIDTH,HEIGHT,32,SDL_OPENGL | flag);
			break;
	}
}

/*
 */
void mouse(int button,int state,int x,int y) {
	if(button == 4) dist -= 10;
	if(button == 5) dist += 10;
	if(dist < 10) dist = 10;
	if(dist > 200) dist = 200;
	psi += (x - WIDTH / 2) * 0.2;
	phi += (y - HEIGHT / 2) * 0.2;
	if(phi < -89) phi = -89;
	if(phi > 89) phi = 89;
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
					keyboard(event.key.keysym.sym);
					break;
				case SDL_MOUSEMOTION:
					mouse(0,0,event.motion.x,event.motion.y);
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					mouse(event.button.button,event.button.state,
						event.button.x,event.button.y);
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
