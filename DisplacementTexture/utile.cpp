/*	utile
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#include "utile.h"

/* shader
 */
Shader::Shader(const char *src) {
	
	char *data;
	FILE *file = fopen(src,"r");
	if(file) {
		fseek(file,0,SEEK_END);
		int size = ftell(file);
		data = new char[size + 1];
		memset(data,0,size + 1);
		fseek(file,0,SEEK_SET);
		fread(data,1,size,file);
		fclose(file);
	} else {
		data = new char[strlen(src) + 1];
		strcpy(data,src);
	}
	
	int error = -1;
	if(!strncmp(data,"!!ARBvp1.0",10)) {
		target = GL_VERTEX_PROGRAM_ARB;
		glGenProgramsARB(1,&id);
		glBindProgramARB(target,id);
		glProgramStringARB(target,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&error);
	} else if(!strncmp(data,"!!ARBfp1.0",10)) {
		target = GL_FRAGMENT_PROGRAM_ARB;
		glGenProgramsARB(1,&id);
		glBindProgramARB(target,id);
		glProgramStringARB(target,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&error);
	} else if(!strncmp(data,"!!FP1.0",7)) {
		target = GL_FRAGMENT_PROGRAM_NV;
		glGenProgramsNV(1,&id);
		glBindProgramNV(target,id);
		glLoadProgramNV(target,id,strlen(data),(GLubyte*)data);
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV,&error);
	} else {
		target = 0;
		id = 0;
		char *s = data;
		while(*s && *s != '\n') s++;
		*s = '\0';
		fprintf(stderr,"unknown program header \"%s\" or error open \"%s\" file\n",data,src);
		delete data;
		return;
	}

	if(error != -1) {
		int line = 0;
		char *s = data;
		while(error-- && *s) if(*s++ == '\n') line++;
		while(s >= data && *s != '\n') s--;
		char *e = ++s;
		while(*e != '\n' && *e != '\0') e++;
		*e = '\0';
		fprintf(stderr,"program error at line %d:\n\"%s\"\n",line,s);
	}

	delete data;
}

Shader::~Shader() {
	if(target == GL_FRAGMENT_PROGRAM_NV) glDeleteProgramsNV(1,&id);
	else glDeleteProgramsARB(1,&id);
}
	
void Shader::enable() {
	glEnable(target);
}

void Shader::disable() {
	glDisable(target);
}

void Shader::bind() {
	if(target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(target,id);
	else glBindProgramARB(target,id);
}

void Shader::envParameter(GLuint index,const vec4 &p) {
	glProgramEnvParameter4fvARB(target,index,p);
}

void Shader::localParameter(GLuint index,const vec4 &p) {
	glProgramLocalParameter4fvARB(target,index,p);
}

void Shader::namedParameter(const char *name,const vec4 &p) {
	glProgramNamedParameter4fvNV(id,strlen(name),(GLubyte*)name,p);
}

/* spline
 */
Spline::Spline(const char *name,int close,float tension,float bias,float continuity) {
	
	memset(this,0,sizeof(Spline));
	
	FILE *file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"Spline: error open \"%s\" file\n",name);
		return;
	}
	
	vec3 v;
	int num = 0;
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) num++;
	vec3 *val = new vec3[num];
	
	num = 0;
	fseek(file,0,SEEK_SET);
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) val[num++] = v;
	fclose(file);
	
	create(val,num,close,tension,bias,continuity);
	
	delete val;
}

Spline::Spline(const vec3 *points,int num,int close,float tension,float bias,float continuity) {
	create(points,num,close,tension,bias,continuity);
}

Spline::~Spline() {
	if(param) delete param;
}

void Spline::create(const vec3 *val,int n,int close,float tension,float bias,float continuity) {
	num = n;
	param = new vec3[num * 4];
	length = 0;
	for(int i = 0; i < num; i++) {
		vec3 prev,cur,next;
		if(i == 0) {
			if(close) prev = val[num - 1];
			else prev = val[i];
			cur = val[i];
			next = val[i + 1];
		} else if(i == num - 1) {
			prev = val[i - 1];
			cur = val[i];
			if(close) next = val[0];
			else next = val[i];
		} else {
			prev = val[i - 1];
			cur = val[i];
			next = val[i + 1];
		}
		vec3 p0 = (cur - prev) * (1.0 + bias);
		vec3 p1 = (next - cur) * (1.0 - bias);
		vec3 r0 = (p0 + (p1 - p0) * 0.5 * (1.0 + continuity)) * (1.0 - tension);
		vec3 r1 = (p0 + (p1 - p0) * 0.5 * (1.0 - continuity)) * (1.0 - tension);
		param[i * 4 + 0] = cur;
		param[i * 4 + 1] = next;
		param[i * 4 + 2] = r0;
		if(i) param[i * 4 - 1] = r1;
		else param[(num - 1) * 4 + 3] = r1;
		length += (next - cur).length();
	}
	for(int i = 0; i < num; i++) {
		vec3 p0 = param[i * 4 + 0];
		vec3 p1 = param[i * 4 + 1];
		vec3 r0 = param[i * 4 + 2];
		vec3 r1 = param[i * 4 + 3];
		param[i * 4 + 0] = p0;
		param[i * 4 + 1] = r0;
		param[i * 4 + 2] = -p0 * 3.0 + p1 * 3.0 - r0 * 2.0 - r1;
		param[i * 4 + 3] = p0 * 2.0 - p1 * 2.0 + r0 + r1;
	}
}

vec3 Spline::operator()(float t) {
	if(!num) return vec3(0,0,0);
	t *= num;
	int i = (int)t;
	t -= i;
	i = (i % num) * 4;
	float t2 = t * t;
	float t3 = t2 * t;
	return param[i + 0] + param[i + 1] * t + param[i + 2] * t2 + param[i + 3] * t3;
}

vec3 Spline::operator()(float speed,float t) {
	return operator()(t * speed / length);
}
vec3 Spline::target(float t) {
	if(!num) return vec3(0,0,1);
	vec3 target = operator()(t + 1.0 / (float)num) - operator()(t);
	target.normalize();
	return target;
}

vec3 Spline::target(float speed,float t) {
	return target(t * speed / length);
}
