/*	utile
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifndef __UTILE_H__
#define __UTILE_H__

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "mathlib.h"

/* shader
 */
class Shader {
public:
	Shader(const char *src);
	~Shader();
	
	void enable();
	void disable();
	void bind();
	
	void envParameter(GLuint index,const vec4 &p);
	void localParameter(GLuint index,const vec4 &p);
	void namedParameter(const char *name,const vec4 &p);
	
	GLuint target;
	GLuint id;
};

/* spline
 */
class Spline {
public:
	Spline(const char *name,int close = 0,float tension = 0.0,float bias = 0.0,float continuity = 0.0);
	Spline(const vec3 *val,int num,int close = 0,float tension = 0.0,float bias = 0.0,float continuity = 0.0);
	~Spline();
	
	vec3 operator()(float t);
	vec3 operator()(float speed,float t);
	vec3 target(float t);
	vec3 target(float speed,float t);

protected:
	void create(const vec3 *val,int num,int close,float tension,float bias,float continuity);
	
	int num;
	vec3 *param;
	float length;
};

/* quick sorting telmplate
 */
template <class T> void quick_sort_process(T *item,int left,int right,int (*comp)(const void *,const void *)) {
	int i = left;
	int j = right;
	T x = item[(left + right) / 2],y;
	do {
		while(comp(&item[i],&x) == -1 && i < right) i++;
		while(comp(&x,&item[j]) == -1 && j > left) j--;
		if(i <= j) {
			y = item[i];
			item[i] = item[j];
			item[j] = y;
			i++;
			j--;
		}
	} while(i <= j);
	if(left < j) quick_sort_process(item,left,j,comp);
	if(i < right) quick_sort_process(item,i,right,comp);
}

template <class T> void quick_sort(T *item,int count,int (*comp)(const void *,const void *)) {
	quick_sort_process(item,0,count - 1,comp);
}

#endif /* __UTILE_H__ */
