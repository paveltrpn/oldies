/*	mesh
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#include "mesh.h"

Mesh::Mesh() {
	memset(this,0,sizeof(Mesh));
}

Mesh::Mesh(const char *name) {
	memset(this,0,sizeof(Mesh));
	load(name);
}

Mesh::~Mesh() {
	if(vertex) delete vertex;
	if(normal) delete normal;
	if(tangent) delete tangent;
	if(binormal) delete binormal;
	if(st) delete st;
	for(int i = 0; i < (int)surface.size(); i++) {
		glDeleteLists(surface[i]->list_id,2);
		delete surface[i];
	}
	surface.clear();
}

/*
 */
void Mesh::load(const char *name) {
	char *ext = strrchr(name,'.');
	if(!ext) {
		fprintf(stderr,"unknown file format");
		return;
	}
	if(!strcmp(ext,".3ds")) load3ds(name);
	else fprintf(stderr,"file \"%s\" not supported\n",name);
	if(vertex) {
		min = vec3(999999,999999,999999);
		max = vec3(-999999,-999999,-999999);
		for(int i = 0; i < (int)surface.size(); i++) {
			Surface *s = surface[i];
			s->list_id = glGenLists(2);
			glNewList(s->list_id,GL_COMPILE);	// ordinary list
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,vertex);
			glNormalPointer(GL_FLOAT,0,normal);
			glTexCoordPointer(2,GL_FLOAT,0,st);
			glDrawArrays(GL_TRIANGLES,s->start_vertex,s->num_vertex);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
			glNewList(s->list_id + 1,GL_COMPILE);	// ppl list
			glEnableVertexAttribArrayARB(0);
			glEnableVertexAttribArrayARB(1);
			glEnableVertexAttribArrayARB(2);
			glEnableVertexAttribArrayARB(3);
			glEnableVertexAttribArrayARB(4);
			glVertexAttribPointerARB(0,3,GL_FLOAT,0,0,vertex);
			glVertexAttribPointerARB(1,3,GL_FLOAT,0,0,normal);
			glVertexAttribPointerARB(2,3,GL_FLOAT,0,0,tangent);
			glVertexAttribPointerARB(3,3,GL_FLOAT,0,0,binormal);
			glVertexAttribPointerARB(4,2,GL_FLOAT,0,0,st);
			glDrawArrays(GL_TRIANGLES,s->start_vertex,s->num_vertex);
			glDisableVertexAttribArrayARB(4);
			glDisableVertexAttribArrayARB(3);
			glDisableVertexAttribArrayARB(2);
			glDisableVertexAttribArrayARB(1);
			glDisableVertexAttribArrayARB(0);
			glEndList();
			s->min = vec3(999999,999999,999999);	// bound box and bound sphere
			s->max = vec3(-999999,-999999,-999999);
			for(int j = s->start_vertex; j < s->start_vertex + s->num_vertex; j++) {
				if(s->min.x > vertex[j].x) s->min.x = vertex[j].x;
				if(s->max.x < vertex[j].x) s->max.x = vertex[j].x;
				if(s->min.y > vertex[j].y) s->min.y = vertex[j].y;
				if(s->max.y < vertex[j].y) s->max.y = vertex[j].y;
				if(s->min.z > vertex[j].z) s->min.z = vertex[j].z;
				if(s->max.z < vertex[j].z) s->max.z = vertex[j].z;
			}
			s->center = (s->max + s->min) / 2.0;
			s->radius = (s->max - s->center).length();
			if(min.x > s->min.x) min.x = s->min.x;
			if(max.x < s->max.x) max.x = s->max.x;
			if(min.y > s->min.y) min.y = s->min.y;
			if(max.y < s->max.y) max.y = s->max.y;
			if(min.z > s->min.z) min.z = s->min.z;
			if(max.z < s->max.z) max.z = s->max.z;
		}
		center = (min + max) / 2;
		radius = (max - center).length();
	}
}

/*
 */
void Mesh::getFrustum() {
	mat4 modelview,projection,clip;
	glGetFloatv(GL_MODELVIEW_MATRIX,modelview);
	glGetFloatv(GL_PROJECTION_MATRIX,projection);
	clip = projection * modelview;
#define CLIP(n,c0,c1,c2,c3) { \
	frustum[n] = vec4(c0,c1,c2,c3); \
	frustum[n] *= 1.0f / vec3(frustum[n]).length(); \
}
	CLIP(0,clip[3]-clip[0],clip[7]-clip[4],clip[11]-clip[8],clip[15]-clip[12])
	CLIP(1,clip[3]+clip[0],clip[7]+clip[4],clip[11]+clip[8],clip[15]+clip[12])
	CLIP(2,clip[3]-clip[1],clip[7]-clip[5],clip[11]-clip[9],clip[15]-clip[13])
	CLIP(3,clip[3]+clip[1],clip[7]+clip[5],clip[11]+clip[9],clip[15]+clip[13])
	CLIP(4,clip[3]-clip[2],clip[7]-clip[6],clip[11]-clip[10],clip[15]-clip[14])
	CLIP(5,clip[3]+clip[2],clip[7]+clip[6],clip[11]+clip[10],clip[15]+clip[14])
#undef CLIP
}

/*
 */
int Mesh::checkBox(const vec3 &min,const vec3 &max) {
	for(int i = 0; i < 6; i++) {
		if(frustum[i] * vec4(min[0],min[1],min[2],1) > 0) continue;
		if(frustum[i] * vec4(min[0],min[1],max[2],1) > 0) continue;
		if(frustum[i] * vec4(min[0],max[1],min[2],1) > 0) continue;
		if(frustum[i] * vec4(min[0],max[1],max[2],1) > 0) continue;
		if(frustum[i] * vec4(max[0],min[1],min[2],1) > 0) continue;
		if(frustum[i] * vec4(max[0],min[1],max[2],1) > 0) continue;
		if(frustum[i] * vec4(max[0],max[1],min[2],1) > 0) continue;
		if(frustum[i] * vec4(max[0],max[1],max[2],1) > 0) continue;
		return -1;
	}
	return 0;
}

/*
 */
void Mesh::setTexture(int unit,GLuint texture_id,int s) {
	if(s < 0) for(int i = 0; i < (int)surface.size(); i++) surface[i]->texture_id[unit] = texture_id;
	else surface[s]->texture_id[unit] = texture_id;
}

/*
 */
void Mesh::bindTexture(int s) {
	GLuint texture[8] = { GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7 };
	if(s < 0) s = 0;
	for(int i = 0; i < 8; i++) {
		if(surface[s]->texture_id[i]) {
			glActiveTexture(texture[i]);
			glBindTexture(GL_TEXTURE_2D,surface[s]->texture_id[i]);
		}
	}
	glActiveTexture(texture[0]);
}

/*
 */
int Mesh::render(int s,int ppl) {
	
	getFrustum();
	
	GLuint texture[8] = { GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7 };
	GLuint texture_id[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	
	int ret = 0;
	
	if(s < 0) {	// render all surfaces
		if(checkBox(min,max) == -1) return 0;
		for(int i = 0; i < (int)surface.size(); i++) {
			Surface *s = surface[i];
			if(checkBox(s->min,s->max) == -1) continue;
			for(int j = 0; j < 8; j++) {
				if(s->texture_id[j] && s->texture_id[j] != texture_id[j]) {
					glActiveTexture(texture[j]);
					glBindTexture(GL_TEXTURE_2D,s->texture_id[j]);
					texture_id[j] = s->texture_id[j];
				}
			}
			if(ppl) glCallList(s->list_id + 1);
			else glCallList(s->list_id);
			ret += s->num_vertex;
		}
	} else {	// render one surface
		if(checkBox(surface[s]->min,surface[s]->max) == -1) return 0;
		for(int i = 0; i < 8; i++) {
			if(surface[s]->texture_id[i]) {
				glActiveTexture(texture[i]);
				glBindTexture(GL_TEXTURE_2D,surface[s]->texture_id[i]);
			}
		}
		if(ppl) glCallList(surface[s]->list_id + 1);
		else glCallList(surface[s]->list_id);
		ret += surface[s]->num_vertex;
	}
	
	glActiveTexture(texture[0]);

	return ret / 3;
}

/*
 */
int Mesh::getStartVertex(int s) {
	if(s < 0) return 0;
	return surface[s]->start_vertex;
}

int Mesh::getNumVertex(int s) {
	if(s < 0) return num_vertex;
	return surface[s]->num_vertex;
}

int Mesh::getNumSurface() {
	return (int)surface.size();
}

int Mesh::getSurface(const char *name) {
	for(int i = 0; i < (int)surface.size(); i++) {
		if(!strcmp(name,surface[i]->name)) return i;
	}
	fprintf(stderr,"error bad surface name \"%s\"\n",name);
	return -1;
}

const char *Mesh::getSurfaceName(int s) const {
	if(s < 0 || s >= (int)surface.size()) {
		fprintf(stderr,"error bad surface\n");
		return NULL;
	}
	return surface[s]->name;
}

float *Mesh::getVertex(int s) {
	if(s < 0) return (float*)vertex;
	return (float*)&vertex[surface[s]->start_vertex];
}

float *Mesh::getNormal(int s) {
	if(s < 0) return (float*)normal;
	return (float*)&normal[surface[s]->start_vertex];
}

float *Mesh::getTangent(int s) {
	if(s < 0) return (float*)tangent;
	return (float*)&tangent[surface[s]->start_vertex];
}

float *Mesh::getBinormal(int s) {
	if(s < 0) return (float*)binormal;
	return (float*)&binormal[surface[s]->start_vertex];
}

float *Mesh::getTexCoord(int s) {
	if(s < 0) return (float*)st;
	return (float*)&st[surface[s]->start_vertex];
}

vec3 Mesh::getMin(int s) {
	if(s < 0) return min;
	return surface[s]->min;
}

vec3 Mesh::getMax(int s) {
	if(s < 0) return max;
	return surface[s]->max;
}

vec3 Mesh::getCenter(int s) {
	if(s < 0) return center;
	return surface[s]->center;
}

float Mesh::getRadius(int s) {
	if(s < 0) return radius;
	return surface[s]->radius;
}

/* loading 3ds files
 */
enum {
	MESH_3DS_CHUNK_MAIN = 0x4d4d,
	MESH_3DS_CHUNK_OBJMESH = 0x3d3d,
	MESH_3DS_CHUNK_OBJBLOCK = 0x4000,
	MESH_3DS_CHUNK_TRIMESH = 0x4100,
	MESH_3DS_CHUNK_VERTLIST = 0x4110,
	MESH_3DS_CHUNK_FACELIST = 0x4120,
	MESH_3DS_CHUNK_MAPLIST = 0x4140,
	MESH_3DS_CHUNK_SMOOTHLIST = 0x4150
};

typedef struct {
	vec3 *vertex;
	int num_vertex;
	vec2 *st;
	int num_st;
	int *face;
	int *smoothgroup;
	int num_face;
	vec3 *normal;
	vec3 *tangent;
	vec3 *binormal;
} mesh_3ds_trimesh_t;

typedef struct {
	char *name;
	mesh_3ds_trimesh_t **trimesh;
	int num_trimesh;
} mesh_3ds_objblock_t;

typedef struct {
	mesh_3ds_objblock_t **objblock;
	int num_objblock;
} mesh_3ds_mesh_t;

typedef int (*mesh_3ds_process_chunk)(FILE *file,int type,int size,void *data);

static int mesh_3ds_skeep_bytes(FILE *file,int bytes) {
	fseek(file,bytes,SEEK_CUR);
	return bytes;
}

static int mesh_3ds_read_string(FILE *file,char *string) {
	int i = 0;
	char *s = string;
	while((*s++ = fgetc(file)) != '\0') i++;
	return ++i;
}

static int mesh_3ds_read_ushort(FILE *file) {
	unsigned short ret;
	fread(&ret,1,sizeof(unsigned short),file);
	return ret;
}

static int mesh_3ds_read_int(FILE *file) {
	int ret;
	fread(&ret,1,sizeof(int),file);
	return ret;
}

static float mesh_3ds_read_float(FILE *file) {
	float ret;
	fread(&ret,1,sizeof(float),file);
	return ret;
}

static int mesh_3ds_read_chunk(FILE *file,mesh_3ds_process_chunk func,void *data) {
	int type = mesh_3ds_read_ushort(file);
	int size = mesh_3ds_read_int(file);
	if(func(file,type,size - 6,data) == -1) mesh_3ds_skeep_bytes(file,size - 6);
	return size;
}

static int mesh_3ds_read_chunks(FILE *file,int bytes,mesh_3ds_process_chunk func,void *data) {
	int bytes_read = 0;
	while(bytes_read < bytes) bytes_read += mesh_3ds_read_chunk(file,func,data);
	if(bytes_read != bytes) fprintf(stderr,"expected %d bytes but read %d\n",bytes,bytes_read);
	return bytes_read;
}

static int mesh_3ds_process_smoothlist(FILE *file,int type,int size,void *data) {
	if(type == MESH_3DS_CHUNK_SMOOTHLIST) {
		mesh_3ds_trimesh_t *trimesh = (mesh_3ds_trimesh_t*)data;
		trimesh->smoothgroup = new int[trimesh->num_face];
		for(int i = 0; i < trimesh->num_face; i++) {
			trimesh->smoothgroup[i] = mesh_3ds_read_int(file);
		}
		return 0;
	}
	return -1;
}

static int mesh_3ds_process_trimesh(FILE *file,int type,int size,void *data) {
	mesh_3ds_trimesh_t *trimesh = (mesh_3ds_trimesh_t*)data;
	if(type == MESH_3DS_CHUNK_VERTLIST) {
		trimesh->num_vertex = mesh_3ds_read_ushort(file);
		trimesh->vertex = new vec3[trimesh->num_vertex];
		for(int i = 0; i < trimesh->num_vertex; i++) {
			trimesh->vertex[i].x = mesh_3ds_read_float(file);
			trimesh->vertex[i].y = mesh_3ds_read_float(file);
			trimesh->vertex[i].z = mesh_3ds_read_float(file);
		}
		return 0;
	} else if(type == MESH_3DS_CHUNK_MAPLIST) {
		trimesh->num_st = mesh_3ds_read_ushort(file);
		trimesh->st = new vec2[trimesh->num_st];
		for(int i = 0; i < trimesh->num_st; i++) {
			trimesh->st[i].x = mesh_3ds_read_float(file);
			trimesh->st[i].y = 1.0f - mesh_3ds_read_float(file);
		}
		return 0;
	} else if(type == MESH_3DS_CHUNK_FACELIST) {
		int bytes_left;
		trimesh->num_face = mesh_3ds_read_ushort(file);
		trimesh->face = new int[trimesh->num_face * 3];
		for(int i = 0; i < trimesh->num_face * 3; i += 3) {
			trimesh->face[i + 0] = mesh_3ds_read_ushort(file);
			trimesh->face[i + 1] = mesh_3ds_read_ushort(file);
			trimesh->face[i + 2] = mesh_3ds_read_ushort(file);
			mesh_3ds_read_ushort(file);
		}
		bytes_left = size - trimesh->num_face * sizeof(unsigned short) * 4 - 2;
		if(bytes_left > 0) mesh_3ds_read_chunks(file,bytes_left,mesh_3ds_process_smoothlist,trimesh);
		return 0;
	}
	return -1;
}

static int mesh_3ds_process_objblock(FILE *file,int type,int size,void *data) {
	if(type == MESH_3DS_CHUNK_TRIMESH) {
		mesh_3ds_objblock_t *objblock = (mesh_3ds_objblock_t*)data;
		objblock->num_trimesh++;
		objblock->trimesh = (mesh_3ds_trimesh_t**)realloc(objblock->trimesh,sizeof(mesh_3ds_trimesh_t) * objblock->num_trimesh);
		objblock->trimesh[objblock->num_trimesh - 1] = new mesh_3ds_trimesh_t;
		mesh_3ds_trimesh_t *trimesh = objblock->trimesh[objblock->num_trimesh - 1];
		memset(trimesh,0,sizeof(mesh_3ds_trimesh_t));
		mesh_3ds_read_chunks(file,size,mesh_3ds_process_trimesh,trimesh);
		return 0;
	}
	return -1;
}

static int mesh_3ds_process_objmesh(FILE *file,int type,int size,void *data) {
	if(type == MESH_3DS_CHUNK_OBJBLOCK) {
		char name[256];
		size -= mesh_3ds_read_string(file,name);
		mesh_3ds_mesh_t *mesh = (mesh_3ds_mesh_t*)data;
		mesh->num_objblock++;
		mesh->objblock = (mesh_3ds_objblock_t**)realloc(mesh->objblock,sizeof(mesh_3ds_objblock_t) * mesh->num_objblock);
		mesh->objblock[mesh->num_objblock - 1] = new mesh_3ds_objblock_t;
		mesh_3ds_objblock_t *objblock = mesh->objblock[mesh->num_objblock - 1];
		memset(objblock,0,sizeof(mesh_3ds_objblock_t));
		objblock->name = strdup(name);
		mesh_3ds_read_chunks(file,size,mesh_3ds_process_objblock,objblock);
		return 0;
	}
	return -1;
}

static int mesh_3ds_process_main(FILE *file,int type,int size,void *data) {
	if(type == MESH_3DS_CHUNK_OBJMESH) {
		mesh_3ds_read_chunks(file,size,mesh_3ds_process_objmesh,data);
		return 0;
	}
	return -1;
}

static void mesh_3ds_trimesh_calc_normals(mesh_3ds_trimesh_t *trimesh) {
	vec3 *normal_face = new vec3[trimesh->num_face];
	vec3 *tangent_face = new vec3[trimesh->num_face];
	vec3 *binormal_face = new vec3[trimesh->num_face];
	int *face = trimesh->face;
	int *vertex_count = new int[trimesh->num_vertex];
	memset(vertex_count,0,sizeof(int) * trimesh->num_vertex);
	int **vertex_face = new int*[trimesh->num_vertex];
	vec3 *vertex = trimesh->vertex;
	vec2 *st = trimesh->st;
	int *smoothgroup = trimesh->smoothgroup;
	trimesh->normal = new vec3[trimesh->num_face * 3];
	memset(trimesh->normal,0,sizeof(vec3) * trimesh->num_face * 3);
	trimesh->tangent = new vec3[trimesh->num_face * 3];
	memset(trimesh->tangent,0,sizeof(vec3) * trimesh->num_face * 3);
	trimesh->binormal = new vec3[trimesh->num_face * 3];
	memset(trimesh->binormal,0,sizeof(vec3) * trimesh->num_face * 3);
	vec3 *normal = trimesh->normal;
	vec3 *tangent = trimesh->tangent;
	vec3 *binormal = trimesh->binormal;
	if(st == NULL) {
		// если текстурных координат нет
		trimesh->st = new vec2[trimesh->num_vertex];
		trimesh->num_st = trimesh->num_vertex;
		st = trimesh->st;
	}
	for(int i = 0; i < trimesh->num_face; i++) {
		// считаем нормали для всех граней
		int j = i * 3;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		vertex_count[v0]++;
		vertex_count[v1]++;
		vertex_count[v2]++;
		normal_face[i].cross(vertex[v1] - vertex[v0],vertex[v2] - vertex[v0]);
		normal_face[i].normalize();
		vec3 e0,e1,n;
		e0 = vec3(0,st[v1].x - st[v0].x,st[v1].y - st[v0].y);
		e1 = vec3(0,st[v2].x - st[v0].x,st[v2].y - st[v0].y);
		for(int k = 0; k < 3; k++) {
			e0.x = vertex[v1][k] - vertex[v0][k];
			e1.x = vertex[v2][k] - vertex[v0][k];
			vec3 cp;
			cp.cross(e0,e1);
			tangent_face[i][k] = -cp[1] / cp[0];
			binormal_face[i][k] = -cp[2] / cp[0];
		}
		tangent_face[i].normalize();
		binormal_face[i].normalize();
		n.cross(tangent_face[i],binormal_face[i]);
		n.normalize();
		binormal_face[i].cross(n,tangent_face[i]);
		//if(normal_face[i].dot(n) < 0) n *= -1;
		//normal_face[i] = n;
	}
	for(int i = 0; i < trimesh->num_vertex; i++) {
		// сколько вершин входит в грань
		vertex_face[i] = new int[vertex_count[i] + 1];
		vertex_face[i][0] = vertex_count[i];
	}
	for(int i = 0; i < trimesh->num_face; i++) {
		// какие вершины входят в грань
		int j = i * 3;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		vertex_face[v0][vertex_count[v0]--] = i;
		vertex_face[v1][vertex_count[v1]--] = i;
		vertex_face[v2][vertex_count[v2]--] = i;
	}
	for(int i = 0; i < trimesh->num_face; i++) {
		// calculate normals of vertex
		int j = i * 3;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		for(int k = 1; k <= vertex_face[v0][0]; k++) {
			int l = vertex_face[v0][k];
			if(l == i || (smoothgroup && smoothgroup[i] & smoothgroup[l])) {
				normal[j + 0] += normal_face[l];
				tangent[j + 0] += tangent_face[l];
				binormal[j + 0] += binormal_face[l];
			}
		}
		for(int k = 1; k <= vertex_face[v1][0]; k++) {
			int l = vertex_face[v1][k];
			if(l == i || (smoothgroup && smoothgroup[i] & smoothgroup[l])) {
				normal[j + 1] += normal_face[l];
				tangent[j + 1] += tangent_face[l];
				binormal[j + 1] += binormal_face[l];
			}
		}
		for(int k = 1; k <= vertex_face[v2][0]; k++) {
			int l = vertex_face[v2][k];
			if(l == i || (smoothgroup && smoothgroup[i] & smoothgroup[l])) {
				normal[j + 2] += normal_face[l];
				tangent[j + 2] += tangent_face[l];
				binormal[j + 2] += binormal_face[l];
			}
		}
	}
	for(int i = 0; i < trimesh->num_face * 3; i++) {
		// нормализуем их
		normal[i].normalize();
		tangent[i].normalize();
		binormal[i].normalize();
	}
	for(int i = 0; i < trimesh->num_vertex; i++) delete vertex_face[i];
	delete normal_face;
	delete tangent_face;
	delete binormal_face;
	delete vertex_count;
	delete vertex_face;
}

/*
 */
void Mesh::load3ds(const char *name) {
	FILE *file;
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"error open \"%s\" file\n",name);
		return;
	}
	int type = mesh_3ds_read_ushort(file);
	int size = mesh_3ds_read_int(file);
	if(type != MESH_3DS_CHUNK_MAIN) {
		fprintf(stderr,"wrong main chunk in file \"%s\"\n",name);
		fclose(file);
		return;
	}
	mesh_3ds_mesh_t *mesh = new mesh_3ds_mesh_t;
	memset(mesh,0,sizeof(mesh_3ds_mesh_t));
	mesh_3ds_read_chunks(file,size - 6,mesh_3ds_process_main,mesh);
	fclose(file);
	num_vertex = 0;
	for(int i = 0; i < mesh->num_objblock; i++) {
		mesh_3ds_objblock_t *objblock = mesh->objblock[i];
		surface.push_back(new Surface);
		memset(surface[i],0,sizeof(Surface));
		strcpy(surface[i]->name,mesh->objblock[i]->name);
		delete mesh->objblock[i]->name;
		int num_objblock_vertex = 0;
		for(int j = 0; j < objblock->num_trimesh; j++) {
			mesh_3ds_trimesh_calc_normals(objblock->trimesh[j]);
			num_objblock_vertex += objblock->trimesh[j]->num_face * 3;
		}
		if(i > 0) surface[i]->start_vertex = surface[i - 1]->start_vertex + surface[i - 1]->num_vertex;
		else surface[i]->start_vertex = 0;
		surface[i]->num_vertex = num_objblock_vertex;
		num_vertex += num_objblock_vertex;
	}
	vertex = new vec3[num_vertex];
	normal = new vec3[num_vertex];
	tangent = new vec3[num_vertex];
	binormal = new vec3[num_vertex];
	st = new vec2[num_vertex];
	for(int i = 0; i < mesh->num_objblock; i++) {
		mesh_3ds_objblock_t *objblock = mesh->objblock[i];
		int num_objblock_vertex = 0;
		for(int j = 0; j < objblock->num_trimesh; j++) {
			mesh_3ds_trimesh_t *trimesh = objblock->trimesh[j];
			// copy vertexes
			for(int k = 0; k < trimesh->num_face; k++) {
				int l = surface[i]->start_vertex + num_objblock_vertex;
				int m = k * 3;
				vertex[l + m + 0] = trimesh->vertex[trimesh->face[m + 0]];
				vertex[l + m + 1] = trimesh->vertex[trimesh->face[m + 1]];
				vertex[l + m + 2] = trimesh->vertex[trimesh->face[m + 2]];
				st[l + m + 0] = trimesh->st[trimesh->face[m + 0]];
				st[l + m + 1] = trimesh->st[trimesh->face[m + 1]];
				st[l + m + 2] = trimesh->st[trimesh->face[m + 2]];
			}
			for(int k = 0; k < trimesh->num_face * 3; k++) {
				int l = surface[i]->start_vertex + num_objblock_vertex;
				normal[l + k] = trimesh->normal[k];
				tangent[l + k] = trimesh->tangent[k];
				binormal[l + k] = trimesh->binormal[k];
			}
			num_objblock_vertex += trimesh->num_face * 3;
			delete trimesh->vertex;
			delete trimesh->st;
			delete trimesh->face;
			if(trimesh->smoothgroup) delete trimesh->smoothgroup;
			delete trimesh->normal;
			delete trimesh->tangent;
			delete trimesh->binormal;
			delete trimesh;
		}
		delete objblock->trimesh;
		delete objblock;
	}
	delete mesh->objblock;
	delete mesh;
}
