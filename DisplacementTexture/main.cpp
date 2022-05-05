/* Displacement Texture demo 20040121
 * 
 * it's a comparison of displacement, offset ppl and ppl bump mappings
 * with horizon shadows
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

/* 4D displasement texture
 */
class DisplacementTexture {
public:
	DisplacementTexture(const char *name,float hMapSize,float hMapScale,int horizon,int vertical);
	~DisplacementTexture();
	
	void bind() { glBindTexture(GL_TEXTURE_3D,id); }
	void bindLookup() { glBindTexture(GL_TEXTURE_CUBE_MAP,lookup_id); }
	
	void createLayer(unsigned char *dest,const vec3 &dir);
	int create(const char *name);
	
	vec3 getCubeVector(int i,int size,int x,int y);
	void createLookup(int size);
	
	float heightWrap(int x,int y) {
		while(x < 0) x += width;
		while(y < 0) y += height;
		if(x >= width) x = x % width;
		if(y >= height) y = y % height;
		return hMap[y * width + x];
	}
	
	int width,height;
	float hMapSize,hMapScale;
	float *hMap;
	
	int horizon,vertical;
	int layers;
	
	GLuint id;
	GLuint lookup_id;
};

/*
 */
DisplacementTexture::DisplacementTexture(const char *name,float hMapSize,float hMapScale,int horizon,int vertical)
	: hMapSize(hMapSize), hMapScale(hMapScale), horizon(horizon), vertical(vertical) {
	
	// load height map
	hMap = NULL;
	unsigned char *h = Texture::load(name,&width,&height);
	if(!h) return;
	
	hMap = new float[width * height];
	float *dest = hMap;
	unsigned char *src = h;
	for(int i = 0; i < width * height; i++) {
		*dest++ = (float)*src / 255.0 * hMapScale;
		src += 4;
	}
	
	delete h;
	
	layers = 0;
	
	id = 0;
	lookup_id = 0;
}

DisplacementTexture::~DisplacementTexture() {
	if(hMap) delete hMap;
	if(id) glDeleteTextures(1,&id);
	if(lookup_id) glDeleteTextures(1,&lookup_id);
}

/*
 */
void DisplacementTexture::createLayer(unsigned char *dest,const vec3 &dir) {
	vec3 d = dir;
	float step = fabs(d.x) > fabs(d.y) ? fabs(d.x) : fabs(d.y);
	d /= step;
	d.z *= hMapSize / (float)width;
	int length = (int)sqrt((float)width * d.x * (float)width * d.x + (float)height * d.y * (float)height * d.y);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			vec3 v(x,y,0);
			int dx = x;
			int dy = y;
			for(int i = 0; i < length; i++) {
				v += d;
				int nx = (int)(v.x + 0.5);
				int ny = (int)(v.y + 0.5);
				float h = heightWrap(nx,ny);
				if(h > v.z) {
					dx = nx;
					dy = ny;
				}
				if(v.z >= hMapScale) break;
			}
			dx -= x;
			dy -= y;
			*dest++ = (unsigned char)((float)dx / (float)width * 128.0) + 128;
			*dest++ = (unsigned char)((float)dy / (float)height * 128.0) + 128;
		}
	}
}

/*
 */
int DisplacementTexture::create(const char *name) {
	static unsigned char *data;
	if(!id) {
		glGenTextures(1,&id);
		glBindTexture(GL_TEXTURE_3D,id);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
		data = new unsigned char[width * height * 2 * horizon * vertical];
		if(!data) GLApp::exit("can not allocate %d Mb of memory");
		FILE *file = fopen(name,"rb");
		if(!file) return horizon * vertical;
		else {
			layers = horizon * vertical;
			fread(data,sizeof(unsigned char) * width * height * 2 * horizon * vertical,1,file);
			glTexImage3D(GL_TEXTURE_3D,0,GL_LUMINANCE8_ALPHA8,width,height,horizon * vertical,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,data);
			delete data;
			fclose(file);
			return 0;
		}
	}
	if(layers != horizon * vertical) {
		int v = layers / horizon;
		int h = layers % horizon;
		vec3 dir;
		dir.x = sin((float)h / (float)horizon * 2 * PI) * cos((float)(v + 1) / (float)(vertical + 1) / 2 * PI);
		dir.y = cos((float)h / (float)horizon * 2 * PI) * cos((float)(v + 1) / (float)(vertical + 1) / 2 * PI);
		dir.z = sin((float)(v + 1) / (float)(vertical + 1) / 2 * PI);
		createLayer(data + width * height * 2 * horizon * v + width * height * 2 * h,dir);
		if(++layers == horizon * vertical) {
			FILE *file = fopen(name,"wb");
			if(file) {
				fwrite(data,sizeof(unsigned char) * width * height * 2 * horizon * vertical,1,file);
				fclose(file);
			}
			glBindTexture(GL_TEXTURE_3D,id);
			glTexImage3D(GL_TEXTURE_3D,0,GL_LUMINANCE8_ALPHA8,width,height,horizon * vertical,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,data);
			delete data;
			return 0;
		}
		return horizon * vertical - layers;
	}
	return 0;
}

/*
 */
vec3 DisplacementTexture::getCubeVector(int i,int size,int x,int y) {
	float s = ((float)x + 0.5) / (float)size * 2.0 - 1.0;
	float t = ((float)y + 0.5) / (float)size * 2.0 - 1.0;
	vec3 v;
	switch(i) {
		case 0: v = vec3(1.0,-t,-s); break;
		case 1: v = vec3(-1.0,-t,s); break;
		case 2: v = vec3(s,1.0,t); break;
		case 3: v = vec3(s,-1.0,-t); break;
		case 4: v = vec3(s,-t,1.0); break;
		case 5: v = vec3(-s,-t,-1.0); break;
	}
	v.normalize();
	return v;
}

void DisplacementTexture::createLookup(int size) {
	glGenTextures(1,&lookup_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP,lookup_id);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	unsigned char *data = new unsigned char[size * size * 3];
	for(int i = 0; i < 6; i++) {
		unsigned char *d = data;
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				vec3 dir = getCubeVector(i,size,x,y);
				vec3 vec(dir.x,dir.y,0);
				vec.normalize();
				float h = acos(vec3(0,1,0) * vec) / (2.0 * PI);
				if(vec.x < 0) h = 1.0 - h;
				float v = acos(dir * vec) / PI;
				*d++ = (unsigned char)((h / (float)vertical + (int)(v * vertical) * 1.0 / (float)vertical) * 255.0);
				*d++ = (unsigned char)((h / (float)vertical + ((int)(v * vertical) + 1) * 1.0 / (float)vertical) * 255.0);
				*d++ = (unsigned char)((v * (float)vertical - (int)(v * vertical)) * 255.0);
			}
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,0,GL_RGB,size,size,0,GL_RGB,GL_UNSIGNED_BYTE,data);
	}
	delete data;
}

/* 3D horizon texture
 */
class HorizonTexture {
public:
	HorizonTexture(const char *name,float hMapSize,float hMapScale,int horizon);
	~HorizonTexture();
	
	void bind() { glBindTexture(GL_TEXTURE_3D,id); }
	void bindLookup() { glBindTexture(GL_TEXTURE_CUBE_MAP,lookup_id); }
	
	void createLayer(unsigned char *dest,const vec2 &dir);
	int create(const char *name);
	
	vec3 getCubeVector(int i,int size,int x,int y);
	void createLookup(int size);
	
	float heightWrap(int x,int y) {
		while(x < 0) x += width;
		while(y < 0) y += height;
		if(x >= width) x = x % width;
		if(y >= height) y = y % height;
		return hMap[y * width + x];
	}
	
	int width,height;
	float hMapSize,hMapScale;
	float *hMap;
	
	int horizon;
	int layers;
	
	GLuint id;
	GLuint lookup_id;
};

/*
 */
HorizonTexture::HorizonTexture(const char *name,float hMapSize,float hMapScale,int horizon)
	: hMapSize(hMapSize), hMapScale(hMapScale), horizon(horizon) {
	
	// load height map
	hMap = NULL;
	unsigned char *h = Texture::load(name,&width,&height);
	if(!h) return;
	
	hMap = new float[width * height];
	float *dest = hMap;
	unsigned char *src = h;
	for(int i = 0; i < width * height; i++) {
		*dest++ = (float)*src / 255.0 * hMapScale;
		src += 4;
	}
	
	delete h;
	
	layers = 0;
	
	id = 0;
	lookup_id = 0;
}

/*
 */
HorizonTexture::~HorizonTexture() {
	if(hMap) delete hMap;
	if(id) glDeleteTextures(1,&id);
	if(lookup_id) glDeleteTextures(1,&lookup_id);
}

/*
 */
void HorizonTexture::createLayer(unsigned char *dest,const vec2 &dir) {
	vec2 d = dir;
	float step = fabs(d.x) > fabs(d.y) ? fabs(d.x) : fabs(d.y);
	d /= step;
	int length = (int)sqrt((float)width * d.x * (float)width * d.x + (float)height * d.y * (float)height * d.y);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			vec2 v(x,y);
			*dest = 0;
			float height_start = heightWrap(x,y);
			float len = 0;
			float dlen =  d.length();
			float angle = 0;
			for(int i = 0; i < length; i++) {
				v += d;
				len += dlen;
				int nx = (int)(v.x + 0.5);
				int ny = (int)(v.y + 0.5);
				float a = (heightWrap(nx,ny) - height_start) / len;
				if(a > angle) angle = a;
			}
			*dest++ = (unsigned char)((1.0 - cos(atan(angle))) * 255.0);
		}
	}
}

/*
 */
int HorizonTexture::create(const char *name) {
	static unsigned char *data;
	if(!id) {
		glGenTextures(1,&id);
		glBindTexture(GL_TEXTURE_3D,id);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_REPEAT);
		data = new unsigned char[width * height * horizon];
		if(!data) GLApp::exit("can not allocate %d Mb of memory");
		FILE *file = fopen(name,"rb");
		if(!file) return horizon;
		else {
			layers = horizon;
			fread(data,sizeof(unsigned char) * width * height * horizon,1,file);
			glTexImage3D(GL_TEXTURE_3D,0,GL_LUMINANCE8,width,height,horizon,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,data);
			delete data;
			fclose(file);
			return 0;
		}
	}
	if(layers != horizon) {
		vec2 dir;
		dir.x = sin((float)layers / (float)horizon * 2 * PI);
		dir.y = cos((float)layers / (float)horizon * 2 * PI);
		createLayer(data + width * height * layers,dir);
		if(++layers == horizon) {
			FILE *file = fopen(name,"wb");
			if(file) {
				fwrite(data,sizeof(unsigned char) * width * height * horizon,1,file);
				fclose(file);
			}
			glBindTexture(GL_TEXTURE_3D,id);
			glTexImage3D(GL_TEXTURE_3D,0,GL_LUMINANCE8,width,height,horizon,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,data);
			delete data;
			return 0;
		}
		return horizon - layers;
	}
	return 0;
}

/*
 */
vec3 HorizonTexture::getCubeVector(int i,int size,int x,int y) {
	float s = ((float)x + 0.5) / (float)size * 2.0 - 1.0;
	float t = ((float)y + 0.5) / (float)size * 2.0 - 1.0;
	vec3 v;
	switch(i) {
		case 0: v = vec3(1.0,-t,-s); break;
		case 1: v = vec3(-1.0,-t,s); break;
		case 2: v = vec3(s,1.0,t); break;
		case 3: v = vec3(s,-1.0,-t); break;
		case 4: v = vec3(s,-t,1.0); break;
		case 5: v = vec3(-s,-t,-1.0); break;
	}
	v.normalize();
	return v;
}

void HorizonTexture::createLookup(int size) {
	glGenTextures(1,&lookup_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP,lookup_id);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	unsigned char *data = new unsigned char[size * size * 2];
	for(int i = 0; i < 6; i++) {
		unsigned char *d = data;
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				vec3 dir = getCubeVector(i,size,x,y);
				vec3 vec(dir.x,dir.y,0);
				vec.normalize();
				float h = acos(vec3(0,1,0) * vec) / (2.0 * PI);
				if(vec.x < 0) h = 1.0 - h;
				*d++ = (unsigned char)(h * 255.0);
				*d++ = (unsigned char)((1.0 - vec * dir) * 255.0);
			}
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,0,GL_LUMINANCE8_ALPHA8,size,size,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,data);
	}
	delete data;
}

/*
 */
class GLAppMain : public GLApp {
public:
	
	int init();
	int idle();
	int render_intro();
	int render_particle(const vec3 &pos,float radius);
	int render();
	
	int loading_complete;
	int pause;
	float time;
	int mode;
	int shadows;
	
	float phi,psi;
	vec3 camera,speed;
	mat4 modelview;
	mat4 projection;
	
	vec3 light;
	Spline *light_path;
	
	Font *font;
	
	Mesh *mesh;
	
	Texture *background_tex;
	
	Texture *light_tex;
	
	Texture *base_tex;
	Texture *normal_tex;
	Texture *height_tex;
	
	DisplacementTexture *displacement_tex;
	HorizonTexture *horizon_tex;
	
	Shader *universal_vp;
	
	Shader *displacement_fp[2];
	Shader *offsetppl_fp[2];
	Shader *ppl_fp[2];
	
	Shader **current_fp;
};

/*
 */
int GLAppMain::init() {
	
	// check hardware
	checkExtension("GL_ARB_texture_cube_map");
	checkExtension("GL_EXT_texture3D");
	checkExtension("GL_ARB_vertex_program");
	checkExtension("GL_ARB_fragment_program");
	
	GLint texture;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB,&texture);
	if(texture < 6) exit("this demo request 6 and more texture image units");
	
	loading_complete = 0;	// see you :)
	pause = 0;
	time = 10;
	mode = 0;
	shadows = 1;
	
	psi = -758.4;
	phi = 6.4;
	camera = vec3(-54.7,-33.2,31.6);
	speed = vec3(0,0,0);
	
	// opengl
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// path
	light_path = new Spline("data/light_path.spl",true);
	
	// font
	font = new Font("data/font.png");
	
	// mesh
	mesh = new Mesh("data/mesh.3ds");
	
	// intro background
	background_tex = new Texture("data/background.jpg");
	
	// light
	light_tex = new Texture("data/light.jpg");
	
	// displasement texture
	displacement_tex = new DisplacementTexture("data/height256x256.png",256,16,32,8);
	
	// horizon texture
	horizon_tex = new HorizonTexture("data/height64x64.png",256,16,16);
	
	// stuf
	base_tex = new Texture("data/base.jpg");
	normal_tex = new Texture("data/normal.jpg");
	height_tex = new Texture("data/height.jpg");
	
	universal_vp = new Shader("data/shaders/universal.vp");
	
	displacement_fp[0] = new Shader("data/shaders/displasement.fp");
	displacement_fp[1] = new Shader("data/shaders/displasementh.fp");
	offsetppl_fp[0] = new Shader("data/shaders/offsetppl.fp");
	offsetppl_fp[1] = new Shader("data/shaders/offsetpplh.fp");
	ppl_fp[0] = new Shader("data/shaders/ppl.fp");
	ppl_fp[1] = new Shader("data/shaders/pplh.fp");
	
	current_fp = displacement_fp;
	
	error();
	
	return 0;
}

/*
 */
int GLAppMain::render_intro() {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,1,-1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_CULL_FACE);
	
	glColor3f(1,1,1);
	background_tex->enable();
	background_tex->bind();
	background_tex->render();
	background_tex->disable();
	
	glColor3f(0.6,0.69,0.67);
	font->printf_center(640,480,320,150,"Loading...");
	
	static int stage = 0;
	
	if(stage == 0) {
		font->printf_center(800,600,400,240,"attempting to load 4D displacement texture");
		stage++;
		return 0;
	}
	
	if(stage == 1) {
		int ret = displacement_tex->create("data/displasement.4d");
		if(ret) {
			static int start_value = 0;
			if(!start_value) start_value = ret;
			font->printf_center(800,600,400,240,"create displacement 4D texture");
			font->printf_center(800,600,400,270,"%.1f%% complete",(float)(start_value - ret) / (float)start_value * 100.0);
			return 0;
		}
		stage++;
		return 0;
	}
	
	if(stage == 2) {
		font->printf_center(800,600,400,240,"create displacement lookup texture");
		stage++;
		return 0;
	}
	
	if(stage == 3) {
		displacement_tex->createLookup(128);
		stage++;
		return 0;
	}
	
	if(stage == 4) {
		font->printf_center(800,600,400,240,"attempting to load 3D horizon texture");
		stage++;
		return 0;
	}
	
	if(stage == 5) {
		int ret = horizon_tex->create("data/horizon.3d");
		if(ret) {
			static int start_value = 0;
			if(!start_value) start_value = ret;
			font->printf_center(800,600,400,240,"create horizon 3D texture");
			font->printf_center(800,600,400,270,"%.1f%% complete",(float)(start_value - ret) / (float)start_value * 100.0);
			return 0;
		}
		stage++;
		return 0;
	}
	
	if(stage == 6) {
		font->printf_center(800,600,400,240,"create horizon lookup texture");
		stage++;
		return 0;
	}
	
	if(stage == 7) {
		horizon_tex->createLookup(128);
		stage++;
		return 0;
	}
	
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	
	time = 10;
	fps = 60.0;
	ifps = 1.0 / fps;
	
	loading_complete = 1;
	
	return 0;
}

/*
 */
int GLAppMain::render_particle(const vec3 &pos,float radius) {
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	mat4 tmodelview = modelview.transpose();
	vec3 x = tmodelview * vec3(radius,0,0);
	vec3 y = tmodelview * vec3(0,radius,0);
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
int GLAppMain::render() {
	
	if(!loading_complete) {
		render_intro();
		return 0;
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	universal_vp->enable();
	universal_vp->bind();
	universal_vp->localParameter(0,vec4(camera));
	universal_vp->localParameter(1,vec4(light,1.0 / 200.0));
	
	current_fp[shadows]->enable();
	current_fp[shadows]->bind();
	
	// set textures
	base_tex->bind();
	glActiveTexture(GL_TEXTURE1);
	normal_tex->bind();
	glActiveTexture(GL_TEXTURE2);
	if(current_fp == offsetppl_fp) height_tex->bind();
	else displacement_tex->bindLookup();
	glActiveTexture(GL_TEXTURE3);
	displacement_tex->bind();
	glActiveTexture(GL_TEXTURE4);
	horizon_tex->bindLookup();
	glActiveTexture(GL_TEXTURE5);
	horizon_tex->bind();
	glActiveTexture(GL_TEXTURE0);
	
	// render scene
	mesh->render(-1,true);
	
	current_fp[shadows]->disable();
	universal_vp->disable();
	
	// light
	glColor3f(1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	light_tex->enable();
	light_tex->bind();
	render_particle(light,2);
	light_tex->disable();
	glDisable(GL_BLEND);
	
	error();

	// info
	glColor3f(0.8,0.8,0.8);
	font->printf(1024,769,10,10,"fps: %.0f",fps);
	glColor3f(0.6,0.6,0.6);
	static char *names[3] = { "displacement", "offset ppl", "ppl" };
	font->printf(1600,1200,15,65,"%s\n%s\n%s\n'h' - horizon shadows toggle\n'1','2','3' - select render mode\n%s bump mapping\nshadows %s",
		glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_VENDOR),
		names[mode],shadows ? "on" : "off");
	glColor3f(1,1,1);

	return 0;
}

/*
 */
int GLAppMain::idle() {
	
	if(!pause) time += ifps;
	
	light = (*light_path)(30,time);
	
	// keyboard events
	if(keys[KEY_ESC]) exit();
	if(keys[' ']) {
		pause = !pause;
		keys[' '] = 0;
	}
	
	if(keys['3']) {
		mode = 2;
		current_fp = ppl_fp;
	} else if(keys['2']) {
		mode = 1;
		current_fp = offsetppl_fp;
	} else if(keys['1']) {
		mode = 0;
		current_fp = displacement_fp;
	}
	
	if(keys['h']) {
		shadows = !shadows;
		keys['h'] = 0;
	}
	
	// camera movement
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
		psi += (mouseX - windowWidth / 2) * 0.2;
		phi += (mouseY - windowHeight / 2) * 0.2;
		if(phi < -89) phi = -89;
		if(phi > 89) phi = 89;
		setPointer(windowWidth / 2,windowHeight / 2);
	} else showPointer(1);
	
	if(keys[KEY_UP] || keys['w']) speed.x += 5 * ifps;
	if(keys[KEY_DOWN] || keys['s']) speed.x -= 5 * ifps;
	if(keys[KEY_LEFT] || keys['a']) speed.y -= 5 * ifps;
	if(keys[KEY_RIGHT] || keys['d']) speed.y += 5 * ifps;
	if(keys[KEY_SHIFT]) speed.z += 5 * ifps;
	if(keys[KEY_CTRL]) speed.z -= 5 * ifps;
	speed -= speed * 0.15;
	
	quat q0,q1;
	q0.set(vec3(0,0,1),-psi);
	q1.set(vec3(0,1,0),phi);
	vec3 dir = (q0 * q1).to_matrix() * vec3(1,0,0);
	vec3 x,y,z;
	x = dir;
	y.cross(dir,vec3(0,0,1));
	y.normalize();
	z.cross(y,x);
	camera += x * speed.x + y * speed.y + z * speed.z;
	modelview.look_at(camera,camera + dir,vec3(0,0,1));
	
	projection.perspective(45,(float)windowWidth / (float)windowHeight,0.1,1000);
	
	return 0;
}

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;

	int fs = 0;
	if(argc > 1 && !strcmp(argv[1],"-fs")) fs = 1;

	glApp->setVideoMode(WIDTH,HEIGHT,fs);
	glApp->setTitle("Displacement Texture demo http://frustum.org");

	if(!glApp->init()) glApp->main();
	
	return 0;
}
