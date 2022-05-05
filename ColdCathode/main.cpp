/* Cold Cathode Lamps demo
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#include <stdio.h>
#include <stdlib.h>
#include "glapp.h"
#include "mathlib.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"
#include "utile.h"

#define WIDTH	1024
#define HEIGHT	768


/*
 */
class CCLamp {	// Cold Cathode Lamp
public:
	CCLamp(const vec4 &color,float radius) : color(color), radius(radius) {
		if(!lamp) init();
	}
	~CCLamp() { }
	
	void render(const vec3 &camera) {
		lamp_vp->enable();
		lamp_vp->bind();
		lamp_vp->localParameter(0,vec4(pos[0]));
		lamp_vp->localParameter(1,vec4(pos[1]));
		lamp_vp->localParameter(2,vec4(camera));
		lamp_fp->enable();
		lamp_fp->bind();
		lamp_fp->localParameter(0,vec4(1.0 / radius,0,0,0));
		glPushMatrix();
		vec3 basis[3];
		basis[0] = pos[1] - pos[0];
		basis[0].normalize();
		basis[1].cross(basis[0],vec3(0,0,1));
		basis[1].normalize();
		basis[2].cross(basis[0],basis[1]);
		basis[2].normalize();
		mat4 m0,m1,m2,m3;
		m0[0] = basis[0].x; m0[4] = basis[1].x; m0[8] = basis[2].x; m0[12] = pos[0].x;
		m0[1] = basis[0].y; m0[5] = basis[1].y; m0[9] = basis[2].y; m0[13] = pos[0].y;
		m0[2] = basis[0].z; m0[6] = basis[1].z; m0[10] = basis[2].z; m0[14] = pos[0].z;
		m0[3] = 0; m0[7] = 0; m0[11] = 0; m0[15] = 1;
		m1.rotation_y(90);
		m2.translate(0,0,-radius);
		m3.scale(radius,radius,(pos[1] - pos[0]).length() + radius * 2);
		mat4 m = m0 * m1 * m2 * m3;
		glMultMatrixf(m);
		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(m);
		glMatrixMode(GL_MODELVIEW);
		lamp->render();
		glPopMatrix();
		lamp_fp->disable();
		lamp_vp->disable();
	}
	
	vec4 color;
	float radius;
	vec3 pos[2];
	
	static void init() {
		lamp = new Mesh("data/cclamp.3ds");
		lamp_vp = new Shader("data/shaders/cclamp.vp");
		lamp_fp = new Shader("data/shaders/cclamp.fp");
	}
	
	static Mesh *lamp;
	static Shader *lamp_vp;
	static Shader *lamp_fp;
};

Mesh *CCLamp::lamp;
Shader *CCLamp::lamp_vp;
Shader *CCLamp::lamp_fp;

/*
 */
class GLAppMain : public GLApp {
public:
	
	int init();
	int idle();
	int render();
	
	int pause;
	float time;
	
	float angle;
	
	float phi,psi,dist;
	vec3 camera;
	mat4 modelview;
	mat4 projection;
	
	Font *font;
	
	CCLamp *lamp[3];
	Texture *lamp_tex[2];
	
	Mesh *mesh;
	Texture *bump_tex;
	
	Shader *light_vp;
	Shader *light_fp;
};

/*
 */
float lamps[16][3] = {
	{ -66, 0, -37 }, { 66, 1, -37 },
	{ 66, 1, -37 }, { 1.4, 0, 78 },
	{ 1.4, 0, 78 }, { -66, 0, -37 },

	{ -1, 0, 50}, { -12, 0, 16 },
	{ -12, 0, 16}, { 7, 0, 24 },
	{ 7, 0, 24 }, { -1.4, 0, -24 },
	
	{ -1.4, 0, -24 }, { 6.4, 0, -9.59 },
	{ -1.4, 0, -24 }, { -3.4, 0, -9.4 }
};

/*
 */
int GLAppMain::init() {
	
	checkExtension("GL_ARB_vertex_program");
	checkExtension("GL_ARB_fragment_program");
	
	pause = 0;
	time = 10;
	
	angle = 0;
	
	psi = 0;
	phi = -35;
	dist = 200;
	
	// opengl
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// font
	font = new Font("data/font.png");
	
	lamp[0] = new CCLamp(vec4(0.1,0.3,0.9,35),4);
	lamp[1] = new CCLamp(vec4(0.6,0.1,0.7,25),4);
	lamp[2] = new CCLamp(vec4(0.6,0.1,0.7,25),8);
	lamp_tex[0] = new Texture("data/cclamp_0.png",Texture::TEXTURE_1D,Texture::CLAMP_TO_EDGE | Texture::TRILINEAR | Texture::MIPMAP_SGIS);
	lamp_tex[1] = new Texture("data/cclamp_1.png",Texture::TEXTURE_1D,Texture::CLAMP_TO_EDGE | Texture::TRILINEAR | Texture::MIPMAP_SGIS);
	
	// mesh
	mesh = new Mesh("data/mesh.3ds");
	bump_tex = new Texture("data/mesh_n.jpg");
	
	// shaders
	light_vp = new Shader("data/shaders/light.vp");
	light_fp = new Shader("data/shaders/light.fp");
	
	error();

	return 0;
}
/*
 */
int GLAppMain::render() {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	// lights
	light_vp->enable();
	light_vp->bind();
	light_vp->localParameter(0,vec4(lamp[0]->pos[0],0));
	light_vp->localParameter(1,vec4(lamp[0]->pos[1],0));
	light_fp->enable();
	light_fp->bind();
	light_fp->localParameter(0,lamp[0]->color);
	
	// first lamp
	bump_tex->bind();
	mesh->render(-1,true);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	// second lamp
	light_vp->localParameter(0,vec4(lamp[1]->pos[0],0));
	light_vp->localParameter(1,vec4(lamp[1]->pos[1],0));
	light_fp->localParameter(0,lamp[1]->color);
	mesh->render(-1,true);
	
	glDisable(GL_BLEND);
	
	light_fp->disable();
	light_vp->disable();
	
	// lamps
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	glDepthMask(GL_FALSE);
	
	lamp_tex[0]->bind();
	lamp[0]->render(camera);
	
	lamp_tex[1]->bind();
	lamp[1]->render(camera);
	
	// high voltage
	glDepthFunc(GL_ALWAYS);
	glViewport(windowWidth - windowWidth / 3,windowHeight - windowHeight / 3,windowWidth / 3,windowHeight / 3);
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0,220,10,0,0,10,0,0,1);
	mat4 transform,itransform;
	transform.rotation_z(angle);
	itransform = transform.inverse();
	glMultMatrixf(transform);
	
	lamp_tex[1]->bind();
	for(int i = 0; i < 8; i++) {
		lamp[2]->pos[0] = lamps[i * 2];
		lamp[2]->pos[1] = lamps[i * 2 + 1];
		lamp[2]->render(itransform * vec3(0,220,10));
	}
	
	glViewport(0,0,windowWidth,windowHeight);
	glDepthFunc(GL_LEQUAL);
	
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	
	error();
	
	// info
	glColor3f(0.8,0.8,0.8);
	font->printf(1024,769,10,10,"fps: %.0f",fps);
	glColor3f(0.6,0.6,0.6);
	font->printf(1600,1200,15,65,"%s\n%s\n%s",
		glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_VENDOR));
	glColor3f(1,1,1);
	
	return 0;
}

/*
 */
int GLAppMain::idle() {
	
	if(!pause) time += ifps;
	
	float a = time * 360 / 6 * DEG2RAD;
	lamp[0]->pos[0] = vec3(cos(a) * 75,sin(a) * 75,25 + 10 * sin(time * 360 / 16 * DEG2RAD));
	lamp[0]->pos[1] = vec3(-cos(a + 0.5) * 40,-sin(a + 0.5) * 40,25);
	
	a = time * 360 / 5 * DEG2RAD;
	lamp[1]->pos[0] = vec3(sin(a) * 80,cos(a) * 80,12 + 8 * sin(time * 360 / 8 * DEG2RAD));
	lamp[1]->pos[1] = vec3(-sin(a + 0.7) * 60,-cos(a + 0.7) * 60,12);
	
	angle = time * 360 / 7;
	
	// keyboard events
	if(keys[KEY_ESC]) exit();
	if(keys[' ']) {
		pause = !pause;
		keys[' '] = 0;
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
		if(phi > -10) phi = -10;
		setPointer(windowWidth / 2,windowHeight / 2);
	} else showPointer(1);
	
	if(mouseButton & BUTTON_UP) dist -= 10;
	if(mouseButton & BUTTON_DOWN) dist += 10;
	if(dist < 20) dist = 20;
	if(dist > 200) dist = 200;
	
	quat q = quat(vec3(0,0,1),psi) * quat(vec3(0,1,0),phi);
	mat4 m0 = q.to_matrix();
	camera = m0 * vec3(dist,0,0);
	modelview.look_at(camera,vec3(0,0,0),vec3(0,0,1));
	
	projection.perspective(45,(float)windowWidth / (float)windowHeight,1,1000);
	
	return 0;
}

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;

	int fs = 0;
	if(argc > 1 && !strcmp(argv[1],"-fs")) fs = 1;
	
	glApp->setVideoMode(WIDTH,HEIGHT,fs);
	glApp->setTitle("Cold Cathode Lamps demo http://frustum.org");
	
	if(!glApp->init()) glApp->main();
	
	return 0;
}
