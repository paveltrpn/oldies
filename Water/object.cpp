/*	object
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#include "object.h"

Object::Object() {
	memset(this,0,sizeof(Object));
}

Object::Object(const char *name) {
	memset(this,0,sizeof(Object));
	load(name);
}

Object::~Object() {
	
}

void Object::render() {
	for(int i = 0; i < num_material; i++) {
		Material *m = material[i];
		if(m->diffuse) {
			m->diffuse->enable();
			m->diffuse->bind();
		}
		if(m->env) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE,GL_ONE);
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);                      
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			m->env->enable();
			m->env->bind();
		}
		for(int j = 0; j < m->num_surface; j++) mesh->render(m->surface[j]);
		if(m->diffuse) {
			m->diffuse->disable();
		}
		if(m->env) {
			m->env->disable();
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_BLEND);
		}
	}
}

void Object::render_mesh() {
	for(int i = 0; i < num_material; i++) {
		Material *m = material[i];
		if(!m->env) for(int j = 0; j < m->num_surface; j++) mesh->render(m->surface[j]);
	}
}

/*
 */
static int get_token(FILE *file,char *str) {
	int quoted = 0;
	char *s = str,c = ' ';
	while(c == ' ' || c == '\n' || c == '\r' || c == '\t') c = fgetc(file);
	if(c == '"') {
		quoted = 1;
		c = fgetc(file);
	}
	while(1) {
		if(c == EOF) {
			*s = '\0';
			return -1;
		}
		if(!quoted) {
			if(c != ' ' && c != '\n' && c != '\r' && c != '\t') *s++ = c;
			else break;
		} else {
			if(c != '"') *s++ = c;
			else break;
		}
		c = fgetc(file);
	}
	*s = '\0';
	return 0;
}

/*
 */
void Object::load(const char *name) {
	try {
		FILE *file = fopen(name,"r");
		if(!file) throw "open object file";
		char buf[1024],path[1024],filename[1024];
		strcpy(path,name);
		char *s = strrchr(path,'/');
		if(s) *s = '\0';
		while(get_token(file,buf) == 0) {
			if(!strcmp(buf,"mesh")) {
				get_token(file,buf);
				if(strcmp(buf,"{")) throw "syntax after mesh";
				get_token(file,buf);
				sprintf(filename,"%s/%s",path,buf);
				mesh = new Mesh(filename);
				get_token(file,buf);
				if(strcmp(buf,"}")) throw "syntax after mesh";
			} else if(!strcmp(buf,"material")) {
				get_token(file,buf);
				if(strcmp(buf,"{")) throw "syntax after mesh";
				Material *mat = (Material*)calloc(1,sizeof(Material));
				material[num_material++] = mat;
				while(get_token(file,buf) == 0) {
					if(!strcmp(buf,"}")) break;
					else if(!strcmp(buf,"diffuse:")) {
						get_token(file,buf);
						sprintf(filename,"%s/%s",path,buf);
						mat->diffuse = new Texture(filename);
					} else if(!strcmp(buf,"env:")) {
						get_token(file,buf);
						sprintf(filename,"%s/%s",path,buf);
						mat->env = new Texture(filename);
					} else if(!strcmp(buf,"surface:")) {
						get_token(file,buf);
						int s = mesh->getSurface(buf);
						if(s == -1) throw("unknown surface in mesh");
						mat->surface[mat->num_surface++] = s;
					} else throw "unknown symbol in material";
				}
			} else throw "unknown symbol";
		}
		fclose(file);
	}
	catch(const char *msg) {
		std::cerr << "error: " << msg << "\n";
		exit(1);
	}
}
