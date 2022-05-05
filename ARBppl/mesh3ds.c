/*  mesh3ds
 *	loading and save 3ds file
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 */

#include "mesh3ds.h"

#define CHUNK_MAIN			0x4d4d
#define CHUNK_OBJMESH		0x3d3d
#define CHUNK_OBJBLOCK		0x4000
#define CHUNK_TRIMESH		0x4100
#define CHUNK_VERTLIST		0x4110
#define CHUNK_FACELIST		0x4120
#define CHUNK_MAPLIST		0x4140
#define CHUNK_SMOOTHLIST	0x4150

typedef struct {
	float x,y,z;
} vector_t;

typedef struct {
	float s,t;
} stmap_t;

typedef struct {
	vector_t *vertex;
	int num_vertex;
	stmap_t *stmap;
	int num_stmap;
	int *face;
	int *smoothgroup;
	vector_t *normal;
	int num_face;
} trimesh_t;

typedef struct {
	trimesh_t *trimesh;
	int num_trimesh;
} mesh_t;

typedef int (*process_chunk)(FILE *file,unsigned short type,int size,void *data);

/*
 */
static void vector_copy(vector_t *a,vector_t *b) {
	b->x = a->x;
	b->y = a->y;
	b->z = a->z;
}

static void vector_add(vector_t *a,vector_t *b,vector_t *c) {
	c->x = a->x + b->x;
	c->y = a->y + b->y;
	c->z = a->z + b->z;
}

static void vector_sub(vector_t *a,vector_t *b,vector_t *c) {
	c->x = a->x - b->x;
	c->y = a->y - b->y;
	c->z = a->z - b->z;
}

static void vector_cross(vector_t *a,vector_t *b,vector_t *c) {
	c->x = a->y * b->z - a->z * b->y;
	c->y = a->z * b->x - a->x * b->z;
	c->z = a->x * b->y - a->y * b->x;
}

static float vector_normalize(vector_t *a,vector_t *b) {
	float length = sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
	if(length) {
		float ilength = 1.0 / length;
		b->x = a->x * ilength;
		b->y = a->y * ilength;
		b->z = a->z * ilength;
		return length;
	}
	b->x = 0.0;
	b->y = 0.0;
	b->z = 0.0;
	return 0.0;
}

/*
 */
static void skeep_bytes(FILE *file,int bytes) {
	fseek(file,bytes,SEEK_CUR);
}

static int skeep_string(FILE *file) {
	int i = 0;
	while(fgetc(file) != '\0') i++;
	return ++i;
}

static int read_ushort(FILE *file) {
	unsigned short ret;
	fread(&ret,1,sizeof(unsigned short),file);
	return ret;
}

static int read_int(FILE *file) {
	int ret;
	fread(&ret,1,sizeof(int),file);
	return ret;
}

static float read_float(FILE *file) {
	float ret;
	fread(&ret,1,sizeof(float),file);
	return ret;
}

static int read_chunk(FILE *file,process_chunk chunk_func,void *data) {
	int type,size;
	type = read_ushort(file);
	size = read_int(file);
	if(chunk_func(file,type,size - 6,data) == -1) skeep_bytes(file,size - 6);
	return size;
}

static int read_chunks(FILE *file,int bytes,process_chunk chunk_func,void *data) {
	int bytes_read = 0;
	while(bytes_read < bytes) bytes_read += read_chunk(file,chunk_func,data);	
	if(bytes_read != bytes) fprintf(stderr,"expected %d bytes but read %d\n",bytes_read,bytes);
	return bytes_read;
}

/*
 */
static int process_smoothlist(FILE *file,unsigned short type,int size,void *data) {
	int i;
	trimesh_t *trimesh = data;
	if(type == CHUNK_SMOOTHLIST) {
		trimesh->smoothgroup = malloc(sizeof(int) * trimesh->num_face);
		for(i = 0; i < trimesh->num_face; i++) {
			trimesh->smoothgroup[i] = read_int(file);
		}
		return 0;
	}
	return -1;
}

static int process_trimesh(FILE *file,unsigned short type,int size,void *data) {
	int i;
	trimesh_t *trimesh = data;
	if(type == CHUNK_VERTLIST) { /* vertlist */
		trimesh->num_vertex = read_ushort(file);
		trimesh->vertex = malloc(sizeof(vector_t) * trimesh->num_vertex);
		for(i = 0; i < trimesh->num_vertex; i++) {
			trimesh->vertex[i].x = read_float(file);
			trimesh->vertex[i].y = read_float(file);
			trimesh->vertex[i].z = read_float(file);
		}
		return 0;
	} else if(type == CHUNK_MAPLIST) { /* maplist */
		trimesh->num_stmap = read_ushort(file);
		trimesh->stmap = malloc(sizeof(stmap_t) * trimesh->num_stmap);
		for(i = 0; i < trimesh->num_stmap; i++) {
			trimesh->stmap[i].s = read_float(file);
			trimesh->stmap[i].t = 1.0 - read_float(file);
		}
		return 0;
	} else if(type == CHUNK_FACELIST) { /* facelist */
		int bytes_left;
		trimesh->num_face = read_ushort(file);
		trimesh->face = malloc(sizeof(int) * trimesh->num_face * 3);
		for(i = 0; i < trimesh->num_face * 3; i += 3) {
			trimesh->face[i + 0] = read_ushort(file);
			trimesh->face[i + 1] = read_ushort(file);
			trimesh->face[i + 2] = read_ushort(file);
			read_ushort(file);
		}
		bytes_left = size - trimesh->num_face * sizeof(unsigned short) * 4 - 2;
		if(bytes_left > 0) read_chunks(file,bytes_left,process_smoothlist,trimesh);
		return 0;
	}
	return -1;
}

static int process_objblock(FILE *file,unsigned short type,int size,void *data) {
	mesh_t *mesh = data;
	if(type == CHUNK_TRIMESH) {
		mesh->num_trimesh++;
		if(mesh->trimesh == NULL) mesh->trimesh = malloc(sizeof(trimesh_t));
		else mesh->trimesh = realloc(mesh->trimesh,sizeof(trimesh_t) * mesh->num_trimesh);
		memset(&mesh->trimesh[mesh->num_trimesh - 1],0,sizeof(trimesh_t));
		read_chunks(file,size,process_trimesh,&mesh->trimesh[mesh->num_trimesh - 1]);
		return 0;
	}
	return -1;
}

static int process_objmesh(FILE *file,unsigned short type,int size,void *data) {
	if(type == CHUNK_OBJBLOCK) {
		size -= skeep_string(file);
		read_chunks(file,size,process_objblock,data);
		return 0;
	}
	return -1;
}

static int process_main(FILE *file,unsigned short type,int size,void *data) {
	if(type == CHUNK_OBJMESH) {
		read_chunks(file,size,process_objmesh,data);
		return 0;
	}
	return -1;
}

/*
 */
static void create_trimesh_load(trimesh_t *trimesh) {
	int i;
	stmap_t *stmap;
	vector_t *vertex,*normal_face,*normal_vertex;
	int *face,*vertex_count,**vertex_face,*smoothgroup;
	/* calc normals */
	normal_face = calloc(1,sizeof(vector_t) * trimesh->num_face);
	normal_vertex = calloc(1,sizeof(vector_t) * trimesh->num_face * 3);
	vertex_count = calloc(1,sizeof(int) * trimesh->num_face * 3);
	vertex_face = calloc(1,sizeof(int*) * trimesh->num_face * 3);
	trimesh->normal = normal_vertex;
	vertex = trimesh->vertex;
	face = trimesh->face;
	smoothgroup = trimesh->smoothgroup;
	for(i = 0; i < trimesh->num_face; i++) {
		int j = i * 3;
		vector_t a,b;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		vertex_count[v0]++;
		vertex_count[v1]++;
		vertex_count[v2]++;
		vector_sub(&vertex[v1],&vertex[v0],&a);
		vector_sub(&vertex[v2],&vertex[v0],&b);
		vector_cross(&a,&b,&normal_face[i]);
		vector_normalize(&normal_face[i],&normal_face[i]);
	}
	for(i = 0; i < trimesh->num_face * 3; i++) {
		vertex_face[i] = malloc(sizeof(int) * (vertex_count[i] + 1));
		vertex_face[i][0] = vertex_count[i];
	}
	for(i = 0; i < trimesh->num_face; i++) {
		int j = i * 3;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		vertex_face[v0][vertex_count[v0]--] = i;
		vertex_face[v1][vertex_count[v1]--] = i;
		vertex_face[v2][vertex_count[v2]--] = i;
	}
	for(i = 0; i < trimesh->num_face; i++) {
		int j = i * 3,k;
		int v0 = face[j + 0];
		int v1 = face[j + 1];
		int v2 = face[j + 2];
		for(k = 1; k <= vertex_face[v0][0]; k++) {
			int l = vertex_face[v0][k];
			if(i == l || (smoothgroup && smoothgroup[i] & smoothgroup[l]))
				vector_add(&normal_vertex[j + 0],&normal_face[l],&normal_vertex[j + 0]);
		}
		for(k = 1; k <= vertex_face[v1][0]; k++) {
			int l = vertex_face[v1][k];
			if(i == l || (smoothgroup && smoothgroup[i] & smoothgroup[l]))
				vector_add(&normal_vertex[j + 1],&normal_face[l],&normal_vertex[j + 1]);
		}
		for(k = 1; k <= vertex_face[v2][0]; k++) {
			int l = vertex_face[v2][k];
			if(i == l || (smoothgroup && smoothgroup[i] & smoothgroup[l]))
				vector_add(&normal_vertex[j + 2],&normal_face[l],&normal_vertex[j + 2]);
		}
	}
	for(i = 0; i < trimesh->num_face * 3; i++) {
		vector_normalize(&normal_vertex[i],&normal_vertex[i]);
		free(vertex_face[i]);
	}
	free(normal_face);
	free(vertex_count);
	free(vertex_face);
	/* create linear arrays for vertex and stmap */
	vertex = calloc(1,sizeof(vector_t) * trimesh->num_face * 3);
	stmap = calloc(1,sizeof(stmap_t) * trimesh->num_face * 3);
	for(i = 0; i < trimesh->num_face * 3; i += 3) {
		vector_copy(&trimesh->vertex[trimesh->face[i + 0]],&vertex[i + 0]);
		vector_copy(&trimesh->vertex[trimesh->face[i + 1]],&vertex[i + 1]);
		vector_copy(&trimesh->vertex[trimesh->face[i + 2]],&vertex[i + 2]);
	}
	free(trimesh->vertex);
	trimesh->vertex = vertex;
	trimesh->num_vertex = trimesh->num_face * 3;
	if(trimesh->stmap) {
		for(i = 0; i < trimesh->num_face * 3; i += 3) {
			stmap[i + 0].s = trimesh->stmap[trimesh->face[i + 0]].s;
			stmap[i + 0].t = trimesh->stmap[trimesh->face[i + 0]].t;
			stmap[i + 1].s = trimesh->stmap[trimesh->face[i + 1]].s;
			stmap[i + 1].t = trimesh->stmap[trimesh->face[i + 1]].t;
			stmap[i + 2].s = trimesh->stmap[trimesh->face[i + 2]].s;
			stmap[i + 2].t = trimesh->stmap[trimesh->face[i + 2]].t;
		}
		free(trimesh->stmap);
	}
	trimesh->stmap = stmap;
	trimesh->num_stmap = trimesh->num_face * 3;
	free(trimesh->face);
	trimesh->face = NULL;
}

/*
 */
static float *create_mesh_load(mesh_t *mesh,int *num_vertex) {
	int i,j;
	float *vertex;
	for(i = 0, *num_vertex = 0; i < mesh->num_trimesh; i++) {
		create_trimesh_load(&mesh->trimesh[i]);
		*num_vertex += mesh->trimesh[i].num_face;
	}
	*num_vertex *= 3;
	vertex = malloc(sizeof(float) * *num_vertex * 8);
	for(i = 0, j = 0; i < mesh->num_trimesh; i++) {
		int k;
		for(k = 0; k < mesh->trimesh[i].num_face * 3; k++, j += 8) {
			vertex[j + 0] = mesh->trimesh[i].vertex[k].x;
			vertex[j + 1] = mesh->trimesh[i].vertex[k].y;
			vertex[j + 2] = mesh->trimesh[i].vertex[k].z;
			vertex[j + 3] = mesh->trimesh[i].normal[k].x;
			vertex[j + 4] = mesh->trimesh[i].normal[k].y;
			vertex[j + 5] = mesh->trimesh[i].normal[k].z;
			vertex[j + 6] = mesh->trimesh[i].stmap[k].s;
			vertex[j + 7] = mesh->trimesh[i].stmap[k].t;
		}
		if(mesh->trimesh[i].vertex) free(mesh->trimesh[i].vertex);
		if(mesh->trimesh[i].normal) free(mesh->trimesh[i].normal);
		if(mesh->trimesh[i].stmap) free(mesh->trimesh[i].stmap);
		if(mesh->trimesh[i].face) free(mesh->trimesh[i].face);
		if(mesh->trimesh[i].smoothgroup) free(mesh->trimesh[i].smoothgroup);
	}
	free(mesh->trimesh);
	return vertex;
}

/*
 */
float *mesh3ds_load(char *name,int *num_vertex) {
	FILE *file;
	mesh_t *mesh;
	float *vertex;
	int type,size;
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"error open %s file\n",name);
		return NULL;
	}
	type = read_ushort(file);
	size = read_int(file);
	if(type != CHUNK_MAIN) {
		fprintf(stderr,"wrong main chunk\n");
		fclose(file);
		return NULL;
	}
	mesh = calloc(1,sizeof(mesh_t));
	read_chunks(file,size - 6,process_main,mesh);
	fclose(file);
	vertex = create_mesh_load(mesh,num_vertex);
	free(mesh);
	return vertex;
}

/*
 */
static void write_ushort(FILE *file,unsigned short i) {
	fwrite(&i,1,sizeof(unsigned short),file);
}

static void write_int(FILE *file,int i) {
	fwrite(&i,1,sizeof(int),file);
}

static void write_data(FILE *file,void *data,int size) {
	fwrite(data,1,size,file);
}

/*
 */
static void *create_trimesh_save(float *vertex,int num_vertex,int *size) {
	int i,num,*buf;
	float *v;
	void *data,*ptr;
	v = malloc(sizeof(float) * 8 * num_vertex);
	memcpy(v,vertex,sizeof(float) * 8 * num_vertex);
	buf = calloc(1,sizeof(int) * num_vertex);
	/* optimizing vertex... */
	for(i = num = 0; i < num_vertex; i++) {
		int j = num - 1;
		while(j >= 0) {
			if(memcmp(&v[j * 8],&v[i * 8],sizeof(float) * 8) == 0) break;
			j--;
		}
		if(j < 0) {
			buf[i] = num;
			memcpy(&v[num * 8],&v[i * 8],sizeof(float) * 8);
			num++;
		} else buf[i] = j;
	}
	/* size trimesh chunk */
	*size = sizeof(int) + sizeof(unsigned short) * 2 + sizeof(float) * 3 * num +
		sizeof(int) + sizeof(unsigned short) * 2 + sizeof(float) * 2 * num +
		sizeof(int) + sizeof(unsigned short) * 2 + sizeof(unsigned short) * 4 * num_vertex / 3 +
		sizeof(int) + sizeof(unsigned short) + sizeof(int) * num_vertex / 3;
	ptr = data = calloc(1,*size);
	/* chunk vertex list */
	*(unsigned short*)ptr = CHUNK_VERTLIST;
	ptr += sizeof(unsigned short);
	*(int*)ptr = sizeof(int) + sizeof(unsigned short) * 2 + sizeof(float) * 3 * num;
	ptr += sizeof(int);
	*(unsigned short*)ptr = num;
	ptr += sizeof(unsigned short);
	for(i = 0; i < num; i++) {
		*(float*)ptr = v[i * 8 + 0];
		ptr += sizeof(float);
		*(float*)ptr = v[i * 8 + 1];
		ptr += sizeof(float);
		*(float*)ptr = v[i * 8 + 2];
		ptr += sizeof(float);
	}
	/* chunk map list */
	*(unsigned short*)ptr = CHUNK_MAPLIST;
	ptr += sizeof(unsigned short);
	*(int*)ptr = sizeof(int) + sizeof(unsigned short) * 2 + sizeof(float) * 2 * num;
	ptr += sizeof(int);
	*(unsigned short*)ptr = num;
	ptr += sizeof(unsigned short);
	for(i = 0; i < num; i++) {
		*(float*)ptr = v[i * 8 + 6];
		ptr += sizeof(float);
		*(float*)ptr = 1.0 - v[i * 8 + 7];
		ptr += sizeof(float);
	}
	/* chunk face list */
	*(unsigned short*)ptr = CHUNK_FACELIST;
	ptr += sizeof(unsigned short);
	*(int*)ptr = sizeof(int) + sizeof(unsigned short) * 2 + sizeof(unsigned short) * 4 * num_vertex / 3 +
		sizeof(int) + sizeof(unsigned short) + sizeof(int) * num_vertex / 3;
	ptr += sizeof(int);
	*(unsigned short*)ptr = num_vertex / 3;
	ptr += sizeof(unsigned short);
	for(i = 0; i < num_vertex / 3; i++) {
		*(unsigned short*)ptr = buf[i * 3 + 0];
		ptr += sizeof(unsigned short);
		*(unsigned short*)ptr = buf[i * 3 + 1];
		ptr += sizeof(unsigned short);
		*(unsigned short*)ptr = buf[i * 3 + 2];
		ptr += sizeof(unsigned short);
		*(unsigned short*)ptr = 7;
		ptr += sizeof(unsigned short);
	}
	/* chunk smooth list in face list chunk */
	*(unsigned short*)ptr = CHUNK_SMOOTHLIST;
	ptr += sizeof(unsigned short);
	*(int*)ptr = sizeof(int) + sizeof(unsigned short) + sizeof(int) * num_vertex / 3;
	ptr += sizeof(int);
	for(i = 0; i < num_vertex / 3; i++) {
		*(int*)ptr = 1;
		ptr += sizeof(int);
	}
	free(buf);
	free(v);
	return data;
}

/*
 */
int mesh3ds_save(char *name,float *vertex,int num_vertex) {
	int size;
	void *data;
	FILE *file;
	file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"error write \"%s\" file\n",name);
		return -1;
	}
	data = create_trimesh_save(vertex,num_vertex,&size);
	write_ushort(file,CHUNK_MAIN);
	write_int(file,size + 6 + 6 + 6 + 6 + 1);
	write_ushort(file,CHUNK_OBJMESH);
	write_int(file,size + 6 + 6 + 6 + 1);
	write_ushort(file,CHUNK_OBJBLOCK);
	write_int(file,size + 6 + 6 + 1);
	write_data(file,"\0",1);	
	write_ushort(file,CHUNK_TRIMESH);
	write_int(file,size + 6);
	write_data(file,data,size);
	fclose(file);
	free(data);
	return 0;
}

/*
 */
void mesh3ds_render(float *vertex,int num_vertex) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(float) * 8,vertex);
	glNormalPointer(GL_FLOAT,sizeof(float) * 8,vertex + 3);
	glTexCoordPointer(2,GL_FLOAT,sizeof(float) * 8,vertex + 6);
	glDrawArrays(GL_TRIANGLES,0,num_vertex);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/*
 */
int mesh3ds_gen_list(float *vertex,int num_vertex) {
	int list;
	list = glGenLists(1);
	glNewList(list,GL_COMPILE);
	mesh3ds_render(vertex,num_vertex);
	glEndList();
	return list;
}
