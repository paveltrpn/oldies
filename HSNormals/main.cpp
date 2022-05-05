/* HSNormals (Hardware Smooth Normals) demo
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#include <stdio.h>
#include <stdlib.h>
#include "glapp.h"
#include "pbuffer.h"
#include "mathlib.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"
#include "utile.h"
#include "metaballs.h"

#define WIDTH	1024
#define HEIGHT	768
#define CUBEMAP	256

/*
 */
class GLAppMain : public GLApp {
public:
	
	int init();
	int idle();
	int render_scene();
	int render_cubemap();
	int render_hsnormals(int smooth);
	int render();
	
	int pause;
	float time;
	
	int flat_normals;
	
	float phi,psi,dist;
	vec3 camera;
	mat4 modelview;
	mat4 projection;
	
	Font *font;
	
	Texture *metal_0_tex;
	Texture *metal_1_tex;
	
	Mesh *ring;
	mat4 ring_transform[3];
	
	MetaBalls *metaballs;
	static vec4 balls[8];
	Spline *path[8];
	vec3 *vertex;
	vec3 *vertex_0;
	vec3 *vertex_1;
	vec3 *vertex_2;
	vec3 *normal;
	int num_vertex;
	
	PBuffer *cubemap_pbuffer;	// 32 bit cubemap pbuffer
	Texture *cubemap_tex;		// cubemap
	
	PBuffer *pbuffer;			// 32 bit hsnormals pbuffer
	Texture *normal_tex;
	
	Shader *normal_vp;			// shaders
	Shader *normal_fp;
	Shader *blur_vp;
	Shader *blur_fp;
	Shader *hsnormals_vp;
	Shader *hsnormals_fp;
	Shader *final_vp;
	Shader *final_fp;
};

vec4 GLAppMain::balls[8];

/*
 */
float field(const vec3 &v) {
	float f = 0;
	for(int i = 0; i < 8; i++) {
		vec4 &b = GLAppMain::balls[i];
		f += b.w / ((b[0] - v[0]) * (b[0] - v[0]) + (b[1] - v[1]) * (b[1] - v[1]) + (b[2] - v[2]) * (b[2] - v[2]));
	}
	return f;
}

/*
 */
int GLAppMain::init() {
	
	checkExtension("GL_ARB_vertex_program");
	checkExtension("GL_ARB_fragment_program");
	checkExtension("GL_ARB_texture_cube_map");
	
	pause = 0;
	time = 10;
	
	flat_normals = 0;
	
	psi = 45;
	phi = -20;
	dist = 450;
	
	// opengl
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// font
	font = new Font("data/font.png");
	
	metal_0_tex = new Texture("data/metal_0.jpg");
	metal_1_tex = new Texture("data/metal_1.jpg");
	
	// mesh
	ring = new Mesh("data/ring.3ds");
	
	// metaballs
	metaballs = new MetaBalls(300,8);
	path[0] = new Spline("data/path/path_0.txt",true);
	path[1] = new Spline("data/path/path_1.txt",true);
	path[2] = new Spline("data/path/path_2.txt",true);
	path[3] = new Spline("data/path/path_3.txt",true);
	path[4] = new Spline("data/path/path_4.txt",true);
	path[5] = new Spline("data/path/path_5.txt",true);
	path[6] = new Spline("data/path/path_6.txt",true);
	path[7] = new Spline("data/path/path_7.txt",true);
	vertex = new vec3[10000 * 3 * 2];	// 10k of triangles
	vertex_0 = new vec3[10000 * 3];
	vertex_1 = new vec3[10000 * 3];
	vertex_2 = new vec3[10000 * 3];
	normal = new vec3[10000 * 3];
	num_vertex = 0;
	
	// 32 bit cubemap pixel buffer
	cubemap_pbuffer = new PBuffer(CUBEMAP,CUBEMAP);
	cubemap_pbuffer->enable();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	cubemap_pbuffer->disable();
	
	cubemap_tex = new Texture(CUBEMAP,CUBEMAP,Texture::TEXTURE_CUBE,Texture::TRILINEAR | Texture::CLAMP_TO_EDGE | Texture::MIPMAP_SGIS);
	
	// 32 bit hsnormals pixel buffer
	pbuffer = new PBuffer(WIDTH / 2,HEIGHT / 2);
	pbuffer->enable();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	pbuffer->disable();
	
	normal_tex = new Texture(WIDTH / 2,HEIGHT / 2,Texture::TEXTURE_RECT,Texture::LINEAR | Texture::CLAMP);
	
	// shaders
	normal_vp = new Shader("data/shaders/normal.vp");
	normal_fp = new Shader("data/shaders/normal.fp");
	blur_vp = new Shader("data/shaders/blur.vp");
	blur_fp = new Shader("data/shaders/blur.fp");
	hsnormals_vp = new Shader("data/shaders/hsnormals.vp");
	hsnormals_fp = new Shader("data/shaders/hsnormals.fp");
	final_vp = new Shader("data/shaders/final.vp");
	final_fp = new Shader("data/shaders/final.fp");
	
	error();

	return 0;
}

/*
 */
int GLAppMain::render_scene() {
	
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
	
	metal_0_tex->enable();
	metal_0_tex->bind();
	for(int i = 0; i < 3; i++) {
		glPushMatrix();
		glMultMatrixf(ring_transform[i]);
		ring->render();
		glPopMatrix();
	}
	metal_0_tex->disable();
	
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	
	return 0;
}

/*
 */
int GLAppMain::render_cubemap() {
	
	GLuint cubemap_id[6] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	
	float cubemap_transform[6][16] = {
		{ 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1 },
		{ 0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 },
		{ -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
	};
	
	cubemap_pbuffer->enable();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90,1.0,1,2000);
	glMatrixMode(GL_MODELVIEW);
	
	for(int i = 0; i < 6; i++) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		glLoadMatrixf(cubemap_transform[i]);

		render_scene();

		cubemap_tex->bind();
		cubemap_tex->copy(cubemap_id[i]);
	}
	
	error();
	
	cubemap_pbuffer->disable();
	
	return 0;
}

/*
 */
int GLAppMain::render_hsnormals(int smooth) {
	
	pbuffer->enable();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// flat normals
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	normal_vp->enable();
	normal_vp->bind();
	normal_fp->enable();
	normal_fp->bind();
	
	// render
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(vec3) * 2,vertex);		// vertex
	glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(vec3) * 2,vertex + 1);	// flat normals
	glDrawArrays(GL_TRIANGLES,0,num_vertex);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
	
	normal_fp->disable();
	normal_vp->disable();
	
	// save it to texture
	normal_tex->bind();
	normal_tex->copy();
	
	if(smooth) {
		// blur it
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
	
		glMatrixMode(GL_PROJECTION);
    	glLoadIdentity();
		glOrtho(-1,1,-1,1,-1,1);
    	glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	
		blur_vp->enable();
		blur_vp->bind();
		blur_fp->enable();
		blur_fp->bind();
	
		normal_tex->bind();
		normal_tex->render();
	
		blur_fp->disable();
		blur_vp->disable();
	
		normal_tex->bind();
		normal_tex->copy();
		
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		
		// smoothing normals
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
    	glLoadMatrixf(projection);
    	glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(modelview);
		
		hsnormals_vp->enable();
		hsnormals_vp->bind();
		hsnormals_vp->envParameter(0,vec4(normal_tex->width,normal_tex->height,0,0));
		hsnormals_fp->enable();
		hsnormals_fp->bind();
		
		normal_tex->bind();
		
		glEnableVertexAttribArrayARB(0);
		glEnableVertexAttribArrayARB(1);
		glEnableVertexAttribArrayARB(2);
		glEnableVertexAttribArrayARB(3);
		glEnableVertexAttribArrayARB(4);
		glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(vec3) * 2,vertex);	// vertex
		glVertexAttribPointerARB(1,3,GL_FLOAT,0,0,vertex_0);				// vertex 0
		glVertexAttribPointerARB(2,3,GL_FLOAT,0,0,vertex_1);				// vertex 1
		glVertexAttribPointerARB(3,3,GL_FLOAT,0,0,vertex_2);				// vertex 1
		glVertexAttribPointerARB(4,3,GL_FLOAT,0,0,normal);					// normal
		glDrawArrays(GL_TRIANGLES,0,num_vertex);
		glDisableVertexAttribArrayARB(4);
		glDisableVertexAttribArrayARB(3);
		glDisableVertexAttribArrayARB(2);
		glDisableVertexAttribArrayARB(1);
		glDisableVertexAttribArrayARB(0);
		
		hsnormals_fp->disable();
		hsnormals_vp->disable();
	
		// read smooth normals
		normal_tex->bind();
		normal_tex->copy();
	}
	
	error();
	
	// disable pbuffer
	pbuffer->disable();

	return 0;
}

/*
 */
int GLAppMain::render() {
	
	render_cubemap();
	render_hsnormals(!flat_normals);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	render_scene();
	
	final_vp->enable();
	final_vp->bind();
	final_vp->envParameter(0,vec4(normal_tex->width,normal_tex->height,0,0));
	final_vp->envParameter(1,vec4(camera,0));
	final_fp->enable();
	final_fp->bind();
	
	normal_tex->bind();
	glActiveTexture(GL_TEXTURE1);
	metal_1_tex->bind();
	glActiveTexture(GL_TEXTURE2);
	cubemap_tex->bind();
	glActiveTexture(GL_TEXTURE0);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(vec3) * 2,vertex);
	glDrawArrays(GL_TRIANGLES,0,num_vertex);
	glDisableClientState(GL_VERTEX_ARRAY);
	
	final_fp->disable();
	final_vp->disable();
	
	error();
	
	// info
	glColor3f(0.75,0.70,0.52);
	font->printf(1024,769,10,10,"fps: %.0f",fps);
	glColor3f(0.60,0.65,0.71);
	font->printf(1600,1200,15,60,"%s\n%s\n%s\n'f' - smooth normals toggle\ntriangles %d",
		glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_VENDOR),
		num_vertex / 3);
	glColor3f(1,1,1);
	
	return 0;
}

/*
 */
int GLAppMain::idle() {
	
	if(!pause) time += ifps;
	
	quat rot;
	rot.set(0,0,1,time * 7);
	ring_transform[0] = rot.to_matrix();
	
	mat4 scale;
	scale.scale(0.95,0.95,0.95);
	rot.set(1,0,0.2,time * 13);
	ring_transform[1] = scale * ring_transform[0] * rot.to_matrix();
	
	rot.set(0,0.2,1,time * 23);
	ring_transform[2] = scale * ring_transform[1] * rot.to_matrix();
	
	float t = time / 50;
	balls[0] = vec4((*path[0])(t),30);
	balls[1] = vec4((*path[1])(t),25);
	balls[2] = vec4((*path[2])(t),30);
	balls[3] = vec4((*path[3])(t),35);
	balls[4] = vec4((*path[4])(t),35);
	balls[5] = vec4((*path[5])(t),35);
	balls[6] = vec4((*path[6])(t),40);
	balls[7] = vec4((*path[7])(t),50);
	
	num_vertex = metaballs->create((vec3*)vertex,field,0.05);
	for(int i = 0; i < num_vertex; i++) {
		int j = i / 3 * 3;
		vertex_0[i] = vertex[(j + 0) * 2];
		vertex_1[i] = vertex[(j + 1) * 2];
		vertex_2[i] = vertex[(j + 2) * 2];
		normal[i] = vec3((i - j == 0) ? 1.0 : 0.0,(i - j == 1) ? 1.0 : 0.0,(i - j == 2) ? 1.0 : 0.0);
	}
	
	// keyboard events
	if(keys[KEY_ESC]) exit();
	if(keys[' ']) {
		pause = !pause;
		keys[' '] = 0;
	}
	
	if(keys['f']) {
		flat_normals = !flat_normals;
		keys['f'] = 0;
	}
	
	// mouse events
	static int look = 0;
	
	if(!look && mouseButton & BUTTON_LEFT) {
		setPointer(windowWidth / 2,windowHeight / 2);
		mouseX = windowWidth / 2;
		mouseY = windowHeight / 2;
		look = 1;
	}
	if(mouseButton & BUTTON_RIGHT) look = 0;
	
	if(look) {
		showPointer(0);
		psi -= (mouseX - windowWidth / 2) * 0.2;
		phi += (mouseY - windowHeight / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 89) phi = 89;
		setPointer(windowWidth / 2,windowHeight / 2);
	} else showPointer(1);
	
	if(mouseButton & BUTTON_UP) dist -= 20;
	if(mouseButton & BUTTON_DOWN) dist += 20;
	if(dist < 200) dist = 200;
	if(dist > 800) dist = 800;
	
	quat q = quat(vec3(0,0,1),psi) * quat(vec3(0,1,0),phi);
	mat4 m0 = q.to_matrix();
	camera = m0 * vec3(dist,0,0);
	modelview.look_at(camera,vec3(0,0,0),vec3(0,0,1));
	
	projection.perspective(45,(float)windowWidth / (float)windowHeight,1,2000);
	
	return 0;
}

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;

	int fs = 0;
	if(argc > 1 && !strcmp(argv[1],"-fs")) fs = 1;
	
	glApp->setVideoMode(WIDTH,HEIGHT,fs);
	glApp->setTitle("HSNormals (Hardware Smooth Normals) demo http://frustum.org");
	
	if(!glApp->init()) glApp->main();
	
	return 0;
}
