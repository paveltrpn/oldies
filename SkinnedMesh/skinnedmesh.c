/*  skinned mesh
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#include "skinnedmesh.h"

static surface_t *skinnedmesh_load_surface_ascii(FILE *file) {
    char buffer[256];
    surface_t *surface;
    surface = (surface_t*)malloc(sizeof(surface_t));
    memset(surface,0,sizeof(surface_t));
    fscanf(file,"%s %s",surface->name,buffer);
    while(fscanf(file,"%s",buffer) != EOF) {
        /* vertex */
        if(!strcmp(buffer,"vertex")) {
            int i;
            fscanf(file,"%d %s",&surface->num_vertex,buffer);
            surface->vertex = (vertex_t*)malloc(sizeof(vertex_t) *
                surface->num_vertex);
            for(i = 0; i < surface->num_vertex; i++) {
                int j;
                vertex_t *vertex = &surface->vertex[i];
                fscanf(file,"%f %f",&vertex->st[0],&vertex->st[1]);
                fscanf(file,"%s %d",buffer,&vertex->num_weight);
                vertex->weight = (weight_t*)malloc(sizeof(weight_t) *
                    vertex->num_weight);
                fscanf(file,"%s",buffer);
                for(j = 0; j < vertex->num_weight; j++) {
                    weight_t *weight = &vertex->weight[j];
                    fscanf(file,"%d %f",&weight->bone,&weight->weight);
                    fscanf(file,"%f %f %f",
                        &weight->xyz[0],
                        &weight->xyz[1],
                        &weight->xyz[2]);
                    fscanf(file,"%f %f %f",
                        &weight->normal[0],
                        &weight->normal[1],
                        &weight->normal[2]);
                }
                fscanf(file,"%s",buffer);
            }
            fscanf(file,"%s",buffer);
        /* face */
        } else if(!strcmp(buffer,"face")) {
            int i;
            fscanf(file,"%d %s",&surface->num_face,buffer);
            surface->face = (face_t*)malloc(sizeof(face_t) * surface->num_face);
            for(i = 0; i < surface->num_face; i++) {
                face_t *face = &surface->face[i];
                fscanf(file,"%d %d %d",&face->v0,&face->v1,&face->v2);
            }
            fscanf(file,"%s",buffer);
        /* } */
        } else if(!strcmp(buffer,"}")) {
            return surface;
        /* error */
        } else {
            fprintf(stderr,"unknown string: '%s'\n",buffer);
            return NULL;
        }
    }
    return NULL;
}

skinnedmesh_t *skinnedmesh_load_ascii(char *name) {
    FILE *file;
    char buffer[256];
    skinnedmesh_t *sm;
    file = fopen(name,"r");
    if(!file) {
        fprintf(stderr,"error load %s file\n",name);
        return NULL;
    }
    sm = (skinnedmesh_t*)malloc(sizeof(skinnedmesh_t));
    memset(sm,0,sizeof(skinnedmesh_t));
    while(fscanf(file,"%s",buffer) != EOF) {
        /* bones */
        if(!strcmp(buffer,"bones")) {
            int i;
            fscanf(file,"%d %s",&sm->num_bone,buffer);
            sm->bone = (bone_t*)malloc(sizeof(bone_t) * sm->num_bone);
            for(i = 0; i < sm->num_bone; i++) {
                int j = 0;
                char c = 0;
                while(c != '"') fread(&c,1,1,file);
                while(c != '"') {
                    sm->bone[i].name[j++] = c;
                    fread(&c,1,1,file);
                }
                sm->bone[i].name[j] = '\0';
            }
            fscanf(file,"%s",buffer);
        /* surface */
        } else if(!strcmp(buffer,"surface")) {
            sm->surface[sm->num_surface++] =
                skinnedmesh_load_surface_ascii(file);
        /* animation */
        } else if(!strcmp(buffer,"animation")) {
            int i;
            fscanf(file,"%d %s",&sm->num_frame,buffer);
            sm->frame = (frame_t**)malloc(sizeof(frame_t*) * sm->num_frame);
            for(i = 0; i < sm->num_frame; i++) {
                int j;
                sm->frame[i] = (frame_t*)malloc(sizeof(frame_t) * sm->num_bone);
                for(j = 0; j < sm->num_bone; j++) {
                    fscanf(file,"%f %f %f",
                        &sm->frame[i][j].xyz[0],
                        &sm->frame[i][j].xyz[1],
                        &sm->frame[i][j].xyz[2]);
                    fscanf(file,"%f %f %f %f",
                        &sm->frame[i][j].rot[0],
                        &sm->frame[i][j].rot[1],
                        &sm->frame[i][j].rot[2],
                        &sm->frame[i][j].rot[3]);
                }
            }
            fscanf(file,"%s",buffer);
        }
    }
    fclose(file);
    return sm;
}

static int find_surface(skinnedmesh_t *sm,char *name) {
    int i;
    for(i = 0; i < sm->num_surface; i++) {
        if(!strcmp(name,sm->surface[i]->name)) return i;
    }
    return -1;
}

void skinnedmesh_load_skin(skinnedmesh_t *sm,char *name) {
    FILE *file;
    int num_surface = 0;
    int surface[MAX_SURFACES];
    char c,buffer[256],*ptr = buffer;
    file = fopen(name,"r");
    if(!file) {
        fprintf(stderr,"error open skin file %s\n",name);
        return;
    }
    while(fread(&c,1,1,file) == 1)
        if(c != ' ' && c != '\t' && c != '\r') {
            if(c == ',') {
                int i;
                *ptr = '\0';
                ptr = buffer;
                i = find_surface(sm,buffer);
                if(i >= 0) surface[num_surface++] = i;
            } else if(c == '\n') {
                int i,width,height,id = 0;
                unsigned char *data = NULL;
                *ptr = '\0';
                ptr = buffer;
                if(strstr(buffer,".jpg")) {
                    data = LoadJPEG(buffer,&width,&height);
                } else if(strstr(buffer,".tga")) {
                    data = LoadTGA(buffer,&width,&height);
                }
                if(data) {
                    glGenTextures(1,&id);
                    glBindTexture(GL_TEXTURE_2D,id);
                    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
                    gluBuild2DMipmaps(GL_TEXTURE_2D,4,width,height,GL_RGBA,
                        GL_UNSIGNED_BYTE,data);
                    free(data);
                }
                for(i = 0; i < num_surface; i++)
                    sm->surface[surface[i]]->textureid = id;
                num_surface = 0;
            } else *ptr++ = c;
        }
    fclose(file);
}

void skinnedmesh_frame(skinnedmesh_t *sm,float frame) {
    int i,frame1,frame2;
    frame1 = (int)frame;
    frame -= frame1;
    if(frame1 >= sm->num_frame) frame1 %= sm->num_frame;
    frame2 = frame1 + 1;
    if(frame2 >= sm->num_frame) frame2 = 0;
    for(i = 0; i < sm->num_bone; i++) {
        float matrix_xyz[16],matrix_rot[16];
        float xyz[3],rot[4],a[3],b[3];
        VectorScale(sm->frame[frame1][i].xyz,1.0 - frame,a);
        VectorScale(sm->frame[frame2][i].xyz,frame,b);
        VectorAdd(a,b,xyz);
        QuaternionSlerp(sm->frame[frame1][i].rot,
            sm->frame[frame2][i].rot,frame,rot);
        MatrixTranslate(xyz[0],xyz[1],xyz[2],matrix_xyz);
        QuaternionToMatrix(rot,matrix_rot);
        MatrixMultiply(matrix_xyz,matrix_rot,sm->bone[i].matrix);
    }
    for(i = 0; i < sm->num_surface; i++) {
        int j;
        surface_t *surface = sm->surface[i];
        for(j = 0; j < surface->num_vertex; j++) {
            int k;
            VectorSet(0,0,0,surface->vertex[j].xyz);
            VectorSet(0,0,0,surface->vertex[j].normal);
            for(k = 0; k < surface->vertex[j].num_weight; k++) {
                float v[3];
                weight_t *weight = &surface->vertex[j].weight[k];
                VectorTransform(weight->xyz,
                    sm->bone[weight->bone].matrix,v);
                VectorScale(v,weight->weight,v);
                VectorAdd(surface->vertex[j].xyz,v,
                    surface->vertex[j].xyz);
                VectorTransformNormal(weight->normal,
                    sm->bone[weight->bone].matrix,v);
                VectorScale(v,weight->weight,v);
                VectorAdd(surface->vertex[j].normal,v,
                    surface->vertex[j].normal);
            }
        }
    }
}

void skinnedmesh_render(skinnedmesh_t *sm) {
    int i;
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	for(i = 0; i < sm->num_surface; i++) {
        surface_t *surface = sm->surface[i];
        glBindTexture(GL_TEXTURE_2D,surface->textureid);
		glVertexPointer(3,GL_FLOAT,sizeof(vertex_t),surface->vertex->xyz);
		glNormalPointer(GL_FLOAT,sizeof(vertex_t),surface->vertex->normal);
		glTexCoordPointer(2,GL_FLOAT,sizeof(vertex_t),surface->vertex->st);
		glDrawElements(GL_TRIANGLES,surface->num_face * 3,
			GL_UNSIGNED_INT,surface->face);
    }
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}
