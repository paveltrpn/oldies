/*	depth shadow demo
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

#include <GL/glew.h>
#include "glapp.h"
#include "font.h"
#include "mathlib.h"
#include "mesh.h"
#include "texture.h"



#define WIDTH	1024
#define HEIGHT	768

/*
 */
class GLAppMain : public GLApp {
public:

	GLuint load_program(GLuint target,const char *name);
	int init();
	int idle();
	int render();
	int render_particle(const vec3 &pos,float radius);
	int render_mesh(Mesh *mesh,const mat4 &transform);
	int render_ppl_mesh(Mesh *mesh,const mat4 &transform);
	int render_depth(int size);
	int enable_texgen();
	int disable_texgen();
	
	float phi,psi,dist;
	vec3 camera;
	mat4 modelview;
	mat4 tmodelview;
	mat4 imodelview;
	
	GLuint default_vp;	// pervertex lighting
	GLuint ppl_vp;		// perpixel lighting
	GLuint ppl_fp;
	
	Mesh *ground;
	Texture *ground_tex_b;
	Texture *ground_tex_n;
	Texture *ground_tex_g;
	mat4 ground_transform;
	
	Mesh *mesh;
	Texture *mesh_tex_b;
	Texture *mesh_tex_n;
	Texture *mesh_tex_g;
	float mesh_angle;
	vec3 mesh_pos;
	mat4 mesh_transform;
	
	float light_angle;
	vec3 light;
	Texture *light_tex;
	
	GLuint depth_fp;	// shadow
	
	int depth_size;
	GLuint depth_tex_id[2];
	GLenum depth_format;
	float shadow_end;
	
	Font *font;
	
	int mode;
	int ppl;
};

/*
 */
GLuint GLAppMain::load_program(GLuint target,const char *name) {
	FILE *file = fopen(name,"r");
	if(!file) {
		std::cerr << "error open file \"" << name << "\"\n";
		return 0;
	}
	fseek(file,0,SEEK_END);
	int size = ftell(file);
	fseek(file,0,SEEK_SET);
	char *data = new char[size + 1];
	fread(data,1,size,file);
	data[size] = '\0';
	fclose(file);
	GLuint program;
	glGenProgramsARB(1,&program);
	glBindProgramARB(target,program);
	glProgramStringARB(target,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
	delete data;
	char *error = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
	if(error && *error) std::cerr << error << "\n";
	return program;
}

/*
 */
int GLAppMain::init() {
	
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	
	default_vp = load_program(GL_VERTEX_PROGRAM_ARB,"data/default.vp");
	
	ppl_vp = load_program(GL_VERTEX_PROGRAM_ARB,"data/ppl.vp");
	ppl_fp = load_program(GL_FRAGMENT_PROGRAM_ARB,"data/ppl.fp");
	
	ground = new Mesh("data/ground.3ds");
	ground_tex_b = new Texture("data/ground_b.jpg");
	ground_tex_n = new Texture("data/ground_n.jpg");
	ground_tex_g = new Texture("data/ground_g.jpg");
	ground_transform.set_identity();
	
	mesh = new Mesh("data/mesh.3ds");
	mesh_tex_b = new Texture("data/mesh_b.jpg");
	mesh_tex_n = new Texture("data/mesh_n.jpg");
	mesh_tex_g = new Texture("data/mesh_g.jpg");
	mesh_transform.set_identity();
	
	light_tex = new Texture("data/light.tga");
	
	depth_fp = load_program(GL_FRAGMENT_PROGRAM_ARB,"data/depth.fp");
	
	depth_size = 512;
	
	int depth_bits;
	glGetIntegerv(GL_DEPTH_BITS,&depth_bits);
	if(depth_bits == 16) depth_format = GL_DEPTH_COMPONENT16_ARB;
	else depth_format = GL_DEPTH_COMPONENT24_ARB;
	
	glGenTextures(2,depth_tex_id);
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,depth_format,depth_size,depth_size,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
	
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[1]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,depth_format,depth_size,depth_size,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
	
	font = new Font("data/font.tga");
	
	mesh_angle = 0;
	light_angle = 0;
	
	psi = 35;
	phi = -35;
	dist = 10;
	
	mode = 0;
	ppl = 1;
	
	shadow_end = 15.0;
	
	return 0;
}

/*
 */
int GLAppMain::render_particle(const vec3 &pos,float radius) {
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	vec3 x = tmodelview * vec3(1,0,0) * radius;
	vec3 y = tmodelview * vec3(0,1,0) * radius;
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,1);
	glVertex3f(-x[0] - y[0],-x[1] - y[1],-x[2] - y[2]);
	glTexCoord2f(1,1);
	glVertex3f(x[0] - y[0],x[1] - y[1],x[2] - y[2]);
	glTexCoord2f(0,0);
	glVertex3f(-x[0] + y[0],-x[1] + y[1],-x[2] + y[2]);
	glTexCoord2f(1,0);
	glVertex3f(x[0] + y[0],x[1] + y[1],x[2] + y[2]);
	glEnd();
	glPopMatrix();
	return 0;
}

/*
 */
int GLAppMain::render_mesh(Mesh *mesh,const mat4 &transform) {

	glEnable(GL_VERTEX_PROGRAM_ARB);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,default_vp);
	
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,0,light.x,light.y,light.z,0);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,1,camera.x,camera.y,camera.z,0);
	
	glPushMatrix();
	glMultMatrixf(transform);
	glMatrixMode(GL_MATRIX0_ARB);
	glPushMatrix();
	glLoadMatrixf(transform);
	mesh->render();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glDisable(GL_VERTEX_PROGRAM_ARB);
	
	glColor3f(1,1,1);
	
	return 0;
}
/*
 */
int GLAppMain::render_ppl_mesh(Mesh *mesh,const mat4 &transform) {

	glEnable(GL_VERTEX_PROGRAM_ARB);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,ppl_vp);
	glVertexAttribPointerARB(0,3,GL_FLOAT,0,0,mesh->getVertex());
	glVertexAttribPointerARB(1,3,GL_FLOAT,0,0,mesh->getNormal());
	glVertexAttribPointerARB(2,3,GL_FLOAT,0,0,mesh->getTangent());
	glVertexAttribPointerARB(3,3,GL_FLOAT,0,0,mesh->getBinormal());
	glVertexAttribPointerARB(4,2,GL_FLOAT,0,0,mesh->getTexCoord());
	
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glEnableVertexAttribArrayARB(4);
	
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,0,light.x,light.y,light.z,0);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,1,camera.x,camera.y,camera.z,0);
	
	glPushMatrix();
	glMultMatrixf(transform);
	glMatrixMode(GL_MATRIX0_ARB);
	glPushMatrix();
	glLoadMatrixf(transform);
	glDrawArrays(GL_TRIANGLES,0,mesh->getNumVertex());
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glDisableVertexAttribArrayARB(0);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(4);
	glDisable(GL_VERTEX_PROGRAM_ARB);
	
	glColor3f(1,1,1);
	
	return 0;
}

/*
 */
int GLAppMain::render_depth(int size) {
	int viewport[4];

	glGetIntegerv(GL_VIEWPORT,viewport);
	glViewport(0,0,size,size);
	glScissor(0,0,size,size);
	glEnable(GL_SCISSOR_TEST);
	
	glClear(GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(80,1.0,1,20);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(light.x,light.y,light.z,mesh_pos.x,mesh_pos.y,mesh_pos.z,0,0,1);
	
	glPushMatrix();
	glMultMatrixf(ground_transform);
	ground->render();
	glPopMatrix();
	
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[0]);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,size,size);
	
	glClear(GL_DEPTH_BUFFER_BIT);
	
	glPushMatrix();
	glMultMatrixf(mesh_transform);
	mesh->render();
	glDepthFunc(GL_GEQUAL);
	mesh->render();
	glDepthFunc(GL_LEQUAL);
	glPopMatrix();
	
	glPushMatrix();
	glMultMatrixf(ground_transform);
	ground->render();
	glPopMatrix();
	
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[1]);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,size,size);
	
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
	glScissor(viewport[0],viewport[1],viewport[2],viewport[3]);
	glDisable(GL_SCISSOR_TEST);

	return 0;
}

/*
 */
int GLAppMain::enable_texgen() {
	glTexGenfv(GL_S,GL_EYE_PLANE,vec4(1,0,0,0));
	glTexGenfv(GL_T,GL_EYE_PLANE,vec4(0,1,0,0));
	glTexGenfv(GL_R,GL_EYE_PLANE,vec4(0,0,1,0));
	glTexGenfv(GL_Q,GL_EYE_PLANE,vec4(0,0,0,1));
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	return 0;
}

int GLAppMain::disable_texgen() {
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	return 0;
}

/*
 */
int GLAppMain::render() {
	
	render_depth(depth_size);
	
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45,(float)windowWidth / (float)windowHeight,0.1,100);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	
	/* ground
	 */
	if(ppl) {
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,ppl_fp);
		glActiveTextureARB(GL_TEXTURE0);
		ground_tex_b->bind();
		glActiveTextureARB(GL_TEXTURE1);
		ground_tex_n->bind();
		glActiveTextureARB(GL_TEXTURE2);
		ground_tex_g->bind();
		glActiveTextureARB(GL_TEXTURE0);
		render_ppl_mesh(ground,ground_transform);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	} else {
		ground_tex_b->enable();
		ground_tex_b->bind();
		render_mesh(ground,ground_transform);
		ground_tex_b->disable();
	}
	
	/* mesh
	 */
	if(ppl) {
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,ppl_fp);
		glActiveTextureARB(GL_TEXTURE0);
		mesh_tex_b->bind();
		glActiveTextureARB(GL_TEXTURE1);
		mesh_tex_n->bind();
		glActiveTextureARB(GL_TEXTURE2);
		mesh_tex_g->bind();
		glActiveTextureARB(GL_TEXTURE0);
		render_ppl_mesh(mesh,mesh_transform);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	} else {
		mesh_tex_b->enable();
		mesh_tex_b->bind();
		render_mesh(mesh,mesh_transform);
		mesh_tex_b->disable();
	}
	
	/* shadow
	 */
	enable_texgen();
	glMatrixMode(GL_TEXTURE);
	glTranslatef(0.5,0.5,0.5);
	glScalef(0.5,0.5,0.5);
	gluPerspective(80,1.0,1,20);
	gluLookAt(light.x,light.y,light.z,mesh_pos.x,mesh_pos.y,mesh_pos.z,0,0,1);
	
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,depth_fp);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,0,20 + 1,1 - 20,2 * 20 * 1,1.0 / shadow_end);
	
	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[0]);
	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D,depth_tex_id[1]);
	glActiveTextureARB(GL_TEXTURE0);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	
	glPushMatrix();
	glMultMatrixf(ground_transform);
	ground->render();
	glPopMatrix();
	
	glDisable(GL_BLEND);

	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	disable_texgen();
	
	/* light
	 */
	light_tex->enable();
	light_tex->bind();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	glDepthMask(GL_FALSE);
	render_particle(light,0.3);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	light_tex->disable();
	
	/* info
	 */
	glColor3f(0,1,1);
	font->printf(640,480,10,10,"fps: %.0f",fps);
	glColor3f(0,0,1);
	font->printf(640,480,11,11,"fps: %.0f",fps);
	glColor3f(0,1,0);
	font->printf(1024,768,15,50,"shadow end %.1f [up/down]\n%s\n%s\n%s\nperpixel light %s\n",
		shadow_end,glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_VENDOR),ppl ? "yes" : "no");
	glColor3f(1,1,1);

	return 0;
}

/*
 */
int GLAppMain::idle() {
	
	if(keys[KEY_ESC]) exit();
	
	if(keys[' ']) {
		mode = mode++;
		if(mode == 4) mode = 0;
		keys[' '] = 0;
	}
	
	if(keys['p']) {
		ppl = !ppl;
		keys['p'] = 0;
	}
	
	if(keys[KEY_UP]) shadow_end += ifps * 8;
	if(keys[KEY_DOWN]) shadow_end -= ifps * 8;
	if(shadow_end < 0.01) shadow_end = 0.01;
	
	if(!mouseButton) {
		psi += (mouseX - windowWidth / 2) * 0.2;
		phi += (mouseY - windowHeight / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 89) phi = 89;
	}
	if(mouseButton & BUTTON_UP) {
		dist -= 2;
		if(dist < 1) dist = 1;
	}
	if(mouseButton & BUTTON_DOWN) {
		dist += 2;
		if(dist > 100) dist = 100;
	}
	
	setPointer(windowWidth / 2,windowHeight / 2);
	
	if(mode == 0 || mode == 2) light_angle += 360 / 4 * ifps;
	float rad = light_angle * DEG2RAD;
	
	light = vec3(sin(rad) * 4 + cos(rad / 5) * 3,cos(rad) * 4 + sin(rad / 3) * 3,3);
	
	if(mode == 0 || mode == 1) mesh_angle += 360 / 6 * ifps;
	
	mat4 matrix;
	matrix.rotation_x(mesh_angle / 3);
	mesh_transform.rotation_z(mesh_angle);
	mesh_transform = matrix * mesh_transform;
	
	rad = mesh_angle * DEG2RAD;
	mesh_pos = vec3(sin(rad) + cos(rad / 3) * 2,cos(rad) + sin(rad / 5) * 3,0);
	matrix.translate(mesh_pos);
	mesh_transform = matrix * mesh_transform;
	
	quat q0,q1,q2;
	vec3 dir = vec3(0,0,0);
	vec3 up = vec3(0,0,1);
	vec3 v = vec3(0,0,1);
	q0.set(v,psi);
	v = vec3(0,1,0);
	q1.set(v,phi);
	q2 = q0 * q1;
	mat4 m0 = q2.to_matrix();
	camera = m0 * vec3(dist,0,0) + dir;
	modelview.lock_at(camera,dir,up);
	
	tmodelview = modelview.transpose();
	imodelview = modelview.inverse();
	
	return 0;
}

/*
 */
int main(int argc,char **argv) {

	GLAppMain *glApp = new GLAppMain;
	
	glApp->setVideoMode(WIDTH,HEIGHT,0);
	glApp->setTitle("Depth Shadow http://frustum.tomsk.ru");
	glApp->showPointer(0);
	glApp->setPointer(glApp->windowWidth / 2,glApp->windowHeight / 2);
	glApp->init();
	glApp->main();
	
	return 0;
}
