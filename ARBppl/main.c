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
#include "texture.h"
#include "mesh3ds.h"
#include "font.h"

#define WIDTH	1024
#define HEIGHT	768

int my_pause;
float fps,phi,psi,dist = 100;		/* фигня всякая */
vec3_t camera; 						/* положение камеры */
matrix_t modelview;					/* матрица камеры */

vec4_t light_0 = { 0, 0, 0, 1 };	/* источника света 0 */
vec3_t color_0 = { 1.0, 0.0, 0.0 };
vec4_t light_1 = { 0, 0, 0, 1 };	/* источника света 1 */
vec3_t color_1 = { 0.0, 0.0, 1.0 };
vec4_t light_2 = { 0, 0, 0, 1 };	/* источника света 2 */
vec3_t color_2 = { 1.0, 1.0, 1.0 };

font_t *font;						/* шрифт */

int texture_1d;						/* текстуры для источника света */
int texture_2d;

int texture;

int mesh;							/* лист меша */
int ground;
int point_vp;						/* вершинная программа */
int default_vp;						/* вершинная программа */

/*
 */
char *load_file(char *name) {
	FILE *file;
	char *data;
	int size;
	file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"error open %s file\n",name);
		return NULL;
	}
	fseek(file,0,SEEK_END);
	size = ftell(file);
	fseek(file,0,SEEK_SET);
	data = calloc(1,size + 1);
	fread(data,1,size,file);
	fclose(file);
	return data;
}

/*
 */
int load_texture(char *name) {
	char path[256];
	int width,height;
	unsigned char *data;
	
	sprintf(path,name,"2d");
	data = texture_load_tga(path,&width,&height);
	if(!data) return -1;
	glGenTextures(1,&texture_2d);
	glBindTexture(GL_TEXTURE_2D,texture_2d);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	free(data);
	
	sprintf(path,name,"1d");
	data = texture_load_tga(path,&width,&height);
	if(!data) return -1;
	glGenTextures(1,&texture_1d);
	glBindTexture(GL_TEXTURE_1D,texture_1d);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,width,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	free(data);

	return 0;
}

/*
 */
int init(void) {
	float *vertex;
	int num_vertex;
	char *data;
	
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	
	vertex = mesh3ds_load("data/mesh.3ds",&num_vertex);
	assert(vertex);
	mesh = mesh3ds_gen_list(vertex,num_vertex);
	free(vertex);
	
	vertex = mesh3ds_load("data/ground.3ds",&num_vertex);
	assert(vertex);
	ground = mesh3ds_gen_list(vertex,num_vertex);
	free(vertex);
	
	font = font_load("data/font.tga");
	assert(font);
	
	texture = texture_load("data/texture.jpg",TEXTURE_TRILINEAR | TEXTURE_MIPMAP_SGIS);
	
	load_texture("data/point_%s.tga");
	
	glGenProgramsARB(1,&point_vp);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,point_vp);
	data = load_file("data/point.vp");
	assert(data);
	glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
	free(data);
	
	glGenProgramsARB(1,&default_vp);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,default_vp);
	data = load_file("data/default.vp");
	assert(data);
	glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
	free(data);
	
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
void enable_register_combiners(float *color) {
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D,texture_1d);
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture_2d);

	glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV,1);
	glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV,color);

	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_A_NV,
		GL_TEXTURE0_ARB,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_B_NV,
		GL_ZERO,GL_UNSIGNED_INVERT_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_C_NV,
		GL_TEXTURE1_ARB,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV,GL_RGB,GL_VARIABLE_D_NV,
		GL_ZERO,GL_UNSIGNED_INVERT_NV,GL_RGB);
	glCombinerOutputNV(GL_COMBINER0_NV,GL_RGB,GL_DISCARD_NV,GL_DISCARD_NV,
		GL_SPARE0_NV,GL_NONE,GL_NONE,GL_FALSE,GL_FALSE,GL_FALSE);

	glFinalCombinerInputNV(GL_VARIABLE_A_NV,GL_SPARE0_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_B_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_C_NV,GL_CONSTANT_COLOR0_NV,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_D_NV,GL_ZERO,GL_UNSIGNED_IDENTITY_NV,GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_G_NV,GL_ZERO,GL_UNSIGNED_INVERT_NV,GL_ALPHA);
	
	glEnable(GL_REGISTER_COMBINERS_NV);
}

/*
 */
void disable_register_combiners(void) {
	
	glDisable(GL_REGISTER_COMBINERS_NV);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_1D);
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
}

/*
 */
void render_ppl(void) {
	glEnable(GL_VERTEX_PROGRAM_ARB);
	
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,point_vp);

	enable_register_combiners(color_0);
	glLightfv(GL_LIGHT0,GL_POSITION,light_0);
	glCallList(mesh);
	glCallList(ground);
	disable_register_combiners();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	enable_register_combiners(color_1);
	glLightfv(GL_LIGHT0,GL_POSITION,light_1);
	glCallList(mesh);
	glCallList(ground);
	disable_register_combiners();
	
	enable_register_combiners(color_2);
	glLightfv(GL_LIGHT0,GL_POSITION,light_2);
	glCallList(mesh);
	glCallList(ground);
	disable_register_combiners();
	
	glBlendFunc(GL_DST_COLOR,GL_ZERO);
	
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,default_vp);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture);
	glCallList(mesh);
	glCallList(ground);
	glDisable(GL_TEXTURE_2D);
	
	glDisable(GL_BLEND);
	
	glDisable(GL_VERTEX_PROGRAM_ARB);
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
	
	render_ppl();
	
	glPointSize(5);
	glBegin(GL_POINTS);
	glColor3fv(color_0);
	glVertex3fv(light_0);
	glColor3fv(color_1);
	glVertex3fv(light_1);
	glColor3fv(color_2);
	glVertex3fv(light_2);
	glEnd();
	glColor3f(1,1,1);
	
	font_printf(font,1600,1200,10,10,"fps %.2f",fps);
	
	SDL_GL_SwapBuffers();
}

/*
 */
void idle(void) {
	float ifps;
	vec3_t v,dir,up;
	vec4_t q0,q1,q2;
	matrix_t m0;
	static float angle_0,angle_1,angle_2;
	
	fps = get_fps();
	ifps = 1.0 / fps;
	if(my_pause) ifps = 0;
	
	angle_0 += DEG2RAD * ifps * 360 / 10;
	angle_1 += DEG2RAD * ifps * 360 / 7;
	angle_2 += DEG2RAD * ifps * 360 / 13;
	
	v_set(sin(angle_0),cos(angle_0),sin(angle_0 * 3) * 0.2,light_0);
	v_scale(light_0,40,light_0);
	light_0[2] += 20;
	
	v_set(sin(angle_1),cos(angle_1),sin(angle_1 * 3) * 0.2,light_1);
	v_scale(light_1,45,light_1);
	light_1[2] += 20;
	
	v_set(sin(angle_2),cos(angle_2),sin(angle_2 * 3) * 0.2,light_2);
	v_scale(light_2,35,light_2);
	light_2[2] += 20;
	
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
		case SDLK_f:
			if(flag == SDL_FULLSCREEN) flag = 0;
			else flag = SDL_FULLSCREEN;
			SDL_SetVideoMode(WIDTH,HEIGHT,32,SDL_OPENGL | flag);
			break;
		case SDLK_SPACE:
			my_pause = !my_pause;
			break;
	}
}

/*
 */
void mouse(int button,int state,int x,int y) {
	if(button == 0) {
		psi += (x - WIDTH / 2) * 0.2;
		phi += (y - HEIGHT / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 89) phi = 89;
	}
	if(button == 4 && state == SDL_PRESSED) {
		dist -= 10;
		if(dist < 20) dist = 20;
	}
	if(button == 5 && state == SDL_PRESSED) {
		dist += 10;
		if(dist > 500) dist = 500;
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
					keyboard(event.key.keysym.sym);
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
