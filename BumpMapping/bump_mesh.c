#include "bump_mesh.h"

/* грузим объект
 */
bump_mesh_t *bump_mesh_load(char *name) {
	int i,j,num_vertex;
	float *vertex = load_3ds(name,&num_vertex);
	bump_mesh_t *bm;
	if(!vertex) return NULL;
	bm = calloc(1,sizeof(bump_mesh_t));
	bm->vertex = calloc(1,sizeof(bump_vertex_t) * num_vertex);
	bm->num_vertex = num_vertex;
	/* копируем координату, нормаль, текстурную координату */
	for(i = 0; i < num_vertex; i++) {
		v_copy(&vertex[i * 8],bm->vertex[i].xyz);
		v_copy(&vertex[i * 8 + 3],bm->vertex[i].normal);
		bm->vertex[i].st[0] = vertex[i * 8 + 6];
		bm->vertex[i].st[1] = vertex[i * 8 + 7];
	}
	/* считаем tangent и binormal */
	/* кусок из nvidia sdk */
	for(i = 0; i < num_vertex; i += 3) {
		float *v0 = bm->vertex[i + 0].xyz;
		float *v1 = bm->vertex[i + 1].xyz;
		float *v2 = bm->vertex[i + 2].xyz;
		
		float *t0 = bm->vertex[i + 0].st;
		float *t1 = bm->vertex[i + 1].st;
		float *t2 = bm->vertex[i + 2].st;
		
		vec3_t e0,e1,cp,a,b,fnormal;
		vec3_t tangent,binormal,normal;
		
		v_sub(v1,v0,a);
		v_sub(v2,v0,b);
		v_cross(a,b,fnormal);
		v_normalize(fnormal,fnormal);
		
		v_set(0,t1[0] - t0[0],t1[1] - t0[1],e0);
		v_set(0,t2[0] - t0[0],t2[1] - t0[1],e1);

		for(j = 0; j < 3; j++) {
			e0[0] = v1[j] - v0[j];
			e1[0] = v2[j] - v0[j];
			v_cross(e0,e1,cp);
			tangent[j] = -cp[1] / cp[0];
			binormal[j] = -cp[2] / cp[0];
		}

		v_normalize(tangent,tangent);
		v_normalize(binormal,binormal);
		v_cross(tangent,binormal,normal);
		v_normalize(normal,normal);
		v_cross(normal,tangent,binormal);

		if(v_dot(normal,fnormal) < epsilon) v_scale(normal,-1,normal);

		for(j = 0; j < 3; j++) {
			v_copy(tangent,bm->vertex[i + j].basis[0]);
			v_copy(binormal,bm->vertex[i + j].basis[1]);
			v_copy(normal,bm->vertex[i + j].basis[2]);
		}
	}
	free(vertex);
	return bm;
}

/* просто отрендим
 */
void bump_mesh_render(bump_mesh_t *bm,float *transform) {
	glPushMatrix();
	glMultMatrixf(transform);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(bump_vertex_t),bm->vertex->xyz);
	glNormalPointer(GL_FLOAT,sizeof(bump_vertex_t),bm->vertex->normal);
	glTexCoordPointer(2,GL_FLOAT,sizeof(bump_vertex_t),bm->vertex->st);
	/* рендерим */
	glDrawArrays(GL_TRIANGLES,0,bm->num_vertex);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopMatrix();
}

void rotate_vector(float *in,float *t,float *b,float *n,float *out) {
	vec3_t v;
	v_copy(in,v);
	out[0] = v_dot(v,t);
	out[1] = v_dot(v,b);
	out[2] = v_dot(v,n);
}

void bump_mesh_render_light(bump_mesh_t *bm,float *transform,float *lightdir) {
	int i;
	matrix_t itransform;
	glPushMatrix();
	glMultMatrixf(transform);
	m_inverse(transform,itransform);
	
	glBegin(GL_TRIANGLES);
	for(i = 0; i < bm->num_vertex; i++) {
		vec3_t v,light,t,b,n,dir;
		glMultiTexCoord2fv(GL_TEXTURE0_ARB,bm->vertex[i].st);
		glMultiTexCoord2fv(GL_TEXTURE1_ARB,bm->vertex[i].st);
		
		/* трансформируем вершину */
		v_copy(bm->vertex[i].xyz,v);
		v_transform(v,itransform,v);
		v_transform(lightdir,transform,light);
		/* поворачиваем базис */
		v_copy(bm->vertex[i].basis[0],t);
		v_transform_normal(t,transform,t);
		v_copy(bm->vertex[i].basis[1],b);
		v_transform_normal(b,transform,b);
		v_copy(bm->vertex[i].basis[2],n);
		v_transform_normal(n,transform,n);
		/* считаем направление на источник света для каждой вершины */
		v_sub(lightdir,v,dir);
		v_normalize(dir,dir);
		rotate_vector(dir,t,b,n,dir);
		
		//glMultiTexCoord3fv(GL_TEXTURE1_ARB,dir);
		
		v_scale(dir,0.5,dir);
		v_set(0.5,0.5,0.5,v);
		v_add(dir,v,dir);
		glColor3fv(dir);
		
		glVertex3fv(bm->vertex[i].xyz);
	}
	glEnd();
	
	glColor3f(1,1,1);
	
	glPopMatrix();
}

static void bump_get_cube_vector(int i,int size,int x,int y,float *v) {
	float s,t;
	s = ((float)x + 0.5) / (float)size * 2.0 - 1.0;
	t = ((float)y + 0.5) / (float)size * 2.0 - 1.0;
	switch(i) {
		case 0: v_set(1.0,-t,-s,v); break;
		case 1: v_set(-1.0,-t,s,v); break;
		case 2: v_set(s,1.0,t,v); break;
		case 3: v_set(s,-1.0,-t,v); break;
		case 4: v_set(s,-t,1.0,v); break;
		case 5: v_set(-s,-t,-1.0,v); break;
	}
	v_normalize(v,v);
}

int bump_create_cube_map(int size) {
	int id,i,x,y;
	vec3_t v;
	unsigned char *data;
	data = malloc(size * size * 3);
	glGenTextures(1,&id);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB,id);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	for(i = 0; i < 6; i++) {
		for(y = 0; y < size; y++) {
			for(x = 0; x < size; x++) {
				bump_get_cube_vector(i,size,x,y,v);
				data[(y * size + x) * 3 + 0] = 128 + 127 * v[0];
				data[(y * size + x) * 3 + 1] = 128 + 127 * v[1];
				data[(y * size + x) * 3 + 2] = 128 + 127 * v[2];
			}
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i,0,GL_RGB,size,size,0,GL_RGB,GL_UNSIGNED_BYTE,data);
	}
	free(data);
	return id;
}

