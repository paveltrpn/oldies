/*	3d texture
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

#include "glapp.h"
#include "font.h"
#include "mathlib.h"
#include "mesh.h"
#include "texture.h"
#include "pbuffer.h"

#define WIDTH	1024
#define HEIGHT	768

/*
 */
class GLAppMain : public GLApp {
public:

	GLuint load_program(GLuint target,const char *name);
	GLuint load_texture(const char *name,int num);
	
	int init();
	int idle();
	int render_particle(const vec3 &pos,float radius);
	int render_ppl_mesh(Mesh *mesh,const mat4 &transform);
	int render_anim_ppl_mesh(Mesh *mesh,const mat4 &transform,float time);
	int render_glow();
	int render();
	
	float phi,psi,dist;
	vec3 camera;
	mat4 modelview;
	
	Mesh *lava;
	Mesh *stone;
	Mesh *light;
	
	GLuint anim_ppl_vp;
	GLuint anim_ppl_fp;
	Texture *anim_base;
	GLuint anim_bump;
	GLuint anim_height;
	
	GLuint ppl_vp;
	GLuint ppl_fp;
	Texture *stone_b;
	Texture *stone_n;
	Texture *stone_g;
	
	GLuint light_fp;
	vec3 light_pos;
	mat4 light_transform;
	float light_angle;
	
	PBuffer *pbuffer;
	GLuint glow_tex;
	
	Font *font;
	
	float time;
	
	int max_bias;
	
	int pause;
	
	int press;
	int oldX,oldY;
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
GLuint GLAppMain::load_texture(const char *name,int depth) {
	
	int width,height;
	unsigned char **image = new unsigned char*[depth];
	for(int i = 0; i < depth; i++) {
		char path[256];
		sprintf(path,name,i);
		image[i] = Texture::load(path,&width,&height);
		if(!image[i]) return 0;
	}
	
	unsigned char *data = new unsigned char[width * height * depth * 4];
	
	unsigned char *dest = data;
	for(int z = 0; z < depth; z++) {
		unsigned char *src = image[z];
		for(int x = 0; x < width; x++) {
			for(int y = 0; y < height; y++) {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
			}
		}
	}
	
	for(int i = 0; i < depth; i++) delete image[i];
	delete image;
	
	GLuint id;
	glGenTextures(1,&id);
	glBindTexture(GL_TEXTURE_3D,id);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA,width,height,depth,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	
	delete data;
	return id;
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
	
	lava = new Mesh("data/lava.3ds");
	stone = new Mesh("data/stone.3ds");
	light = new Mesh("data/light.3ds");
	
	anim_ppl_vp = load_program(GL_VERTEX_PROGRAM_ARB,"data/anim_ppl.vp");
	anim_ppl_fp = load_program(GL_FRAGMENT_PROGRAM_ARB,"data/anim_ppl.fp");
	anim_base = new Texture("data/anim_base.jpg",Texture::TRILINEAR | Texture::MIPMAP_SGIS | Texture::CLAMP,Texture::TEXTURE_1D);
	anim_bump = load_texture("data/bump/%02d.jpg",32);
	anim_height = load_texture("data/height/%02d.jpg",32);
	
	ppl_vp = load_program(GL_VERTEX_PROGRAM_ARB,"data/ppl.vp");
	ppl_fp = load_program(GL_FRAGMENT_PROGRAM_ARB,"data/ppl.fp");
	
	stone_b = new Texture("data/stone_b.jpg");
	stone_n = new Texture("data/stone_n.jpg");
	stone_g = new Texture("data/stone_g.jpg");
	
	light_fp = load_program(GL_FRAGMENT_PROGRAM_ARB,"data/light.fp");
	
	pbuffer = new PBuffer(512,512);
	pbuffer->enable();
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	pbuffer->disable();
	
	glGenTextures(1,&glow_tex);
	glBindTexture(GL_TEXTURE_2D,glow_tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,pbuffer->width,pbuffer->height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
	
	font = new Font("data/font.tga");
	
	time = 0;
	light_angle = 0;
	
	max_bias = 6;
	
	psi = 35;
	phi = 35;
	dist = 40;
	
	pause = 0;
	
	press = 0;
	
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
	
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,0,light_pos.x,light_pos.y,light_pos.z,0);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,1,camera.x,camera.y,camera.z,0);
	
	glPushMatrix();
	glMultMatrixf(transform);
	glMatrixMode(GL_MATRIX0_ARB);
	glLoadMatrixf(transform);
	glDrawArrays(GL_TRIANGLES,0,mesh->getNumVertex());
	glLoadIdentity();
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
int GLAppMain::render_anim_ppl_mesh(Mesh *mesh,const mat4 &transform,float t) {

	glEnable(GL_VERTEX_PROGRAM_ARB);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,anim_ppl_vp);
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
	
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,0,light_pos.x,light_pos.y,light_pos.z,0);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,1,camera.x,camera.y,camera.z,0);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,2,cos(time / 8) / 2,sin(time / 8) / 2,t,0);
	
	glPushMatrix();
	glMultMatrixf(transform);
	glMatrixMode(GL_MATRIX0_ARB);
	glLoadMatrixf(transform);
	glDrawArrays(GL_TRIANGLES,0,mesh->getNumVertex());
	glLoadIdentity();
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
int GLAppMain::render_glow() {
	pbuffer->enable();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45,(float)windowWidth / (float)windowHeight,1,500);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	
	glColor3f(0,0,0);
	stone->render();
	
	glColor3f(1,0.5,0);
	lava->render();
	glPushMatrix();
	glMultMatrixf(light_transform);
	light->render();
	glPopMatrix();
	
	glBindTexture(GL_TEXTURE_2D,glow_tex);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,pbuffer->width,pbuffer->height);
	
	pbuffer->disable();

	return 0;
}

/*
 */
int GLAppMain::render() {
	
	render_glow();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45,(float)windowWidth / (float)windowHeight,1,500);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview.mat);
	
	/*
	 */
	mat4 transform;
	transform.set_identity();
	
	/* stone
	 */
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,ppl_fp);
	glActiveTextureARB(GL_TEXTURE0);
	stone_b->bind();
	glActiveTextureARB(GL_TEXTURE1);
	stone_n->bind();
	glActiveTextureARB(GL_TEXTURE2);
	stone_g->bind();
	glActiveTextureARB(GL_TEXTURE0);
	render_ppl_mesh(stone,transform);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	/* glow
	 */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,glow_tex);
	for(int bias = 0; bias < max_bias; bias++) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,GL_TEXTURE_LOD_BIAS_EXT,0.5 + (float)bias);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0,0);
		glVertex2f(-1,-1);
		glTexCoord2f(1,0);
		glVertex2f(1,-1);
		glTexCoord2f(0,1);
		glVertex2f(-1,1);
		glTexCoord2f(1,1);
		glVertex2f(1,1);
		glEnd();
	}
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,GL_TEXTURE_LOD_BIAS_EXT,0);
	glDisable(GL_TEXTURE_2D);
	
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	/* lava
	 */
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,anim_ppl_fp);
	glActiveTextureARB(GL_TEXTURE0);
	anim_base->bind();
	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D,anim_bump);
	glActiveTextureARB(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D,anim_height);
	glActiveTextureARB(GL_TEXTURE0);
	float t = time / 8;
	render_anim_ppl_mesh(lava,transform,(t - ((int)t % 32)));
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	/* light 
	 */
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,light_fp);
	glActiveTextureARB(GL_TEXTURE0);
	anim_base->bind();
	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D,anim_height);
	glActiveTextureARB(GL_TEXTURE0);
	render_anim_ppl_mesh(light,light_transform,(t - ((int)t % 32)));
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	error();
	
	/* info
	 */
	glColor3f(1,1,0);
	font->printf(640,480,10,10,"fps: %.0f",fps);
	glColor3f(1,0.5,0);
	font->printf(1024,768,15,50,"glow: %d [up/down]\n%s\n%s\n%s",max_bias,glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_VENDOR));
	glColor3f(1,1,1);

	return 0;
}

/*
 */
int GLAppMain::idle() {
	
	if(!(mouseButton & BUTTON_LEFT)) press = 0;
	if(press == 1) {
		psi += (mouseX - oldX) * 0.2;
		phi += (mouseY - oldY) * 0.2;
		if(phi < 0) phi = 0;
		if(phi > 89) phi = 89;
	}
	if(mouseButton & BUTTON_LEFT) {
		press = 1;
		oldX = mouseX;
		oldY = mouseY;
	}
	if(mouseButton & BUTTON_UP) {
		dist -= 2;
		if(dist < 1) dist = 1;
	}
	if(mouseButton & BUTTON_DOWN) {
		dist += 2;
		if(dist > 100) dist = 100;
	}
	
	if(keys[KEY_ESC]) exit();
	
	if(keys[' ']) {
		pause = !pause;
		keys[' '] = 0;
	}
	
	if(keys[KEY_UP]) {
		max_bias++;
		keys[KEY_UP] = 0;
	}
	if(keys[KEY_DOWN]) {
		max_bias--;
		keys[KEY_DOWN] = 0;
		if(max_bias < 0) max_bias = 0;
	}
	
	time += ifps;
	
	if(!pause) light_angle += 360 / 4 * ifps;
	float rad = light_angle * DEG2RAD;
	light_pos = vec3(sin(rad) * 16 + cos(rad / 5) * 8,cos(rad) * 16 + sin(rad / 3) * 8,24);
	
	light_transform.translate(light_pos);
	
	quat q0,q1,q2;
	vec3 dir = vec3(0,0,8);
	vec3 up = vec3(0,0,1);
	vec3 v = vec3(0,0,1);
	q0.set(v,-psi);
	v = vec3(0,1,0);
	q1.set(v,-phi);
	q2 = q0 * q1;
	mat4 m0 = q2.to_matrix();
	camera = m0 * vec3(dist,0,0) + dir;
	modelview.lock_at(camera,dir,up);
	
	return 0;
}

/*
 */
int main(int argc,char **argv) {

	GLAppMain *glApp = new GLAppMain;
	
	glApp->setVideoMode(WIDTH,HEIGHT,0);
	glApp->setTitle("Glow http://frustum.tomsk.ru");
	glApp->showPointer(1);
	glApp->init();
	glApp->main();
	
	return 0;
}
