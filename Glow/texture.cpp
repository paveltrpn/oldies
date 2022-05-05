/*	texture
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#include "texture.h"

/*
 */
Texture::Texture() {

}

/*
 */
Texture::Texture(const char *name,int flag,int texture_target) {
	load(name,flag,texture_target);
}

/*
 */
Texture::~Texture() {
	glDeleteTextures(1,&id);
}

/*
 */
void Texture::load(const char *name,int flag,int texture_target) {
	int width,height;
	unsigned char *data = load(name,&width,&height);
	if(!data) return;
	target = texture_target;
	glGenTextures(1,&id);
	glBindTexture(target,id);
	glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	if(flag & TRILINEAR) {
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	}
	if(flag & CLAMP) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP);
	} else if(flag & CLAMP_TO_EDGE) {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_REPEAT);
	}
	if(flag & MIPMAP_SGIS) {
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		if(texture_target == TEXTURE_1D) {
			glTexImage1D(target,0,GL_RGBA,width,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
		} else {
			glTexImage2D(target,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
		}
	} else {
		if(texture_target == TEXTURE_1D) {
			gluBuild1DMipmaps(target,GL_RGBA,width,GL_RGBA,GL_UNSIGNED_BYTE,data);
		} else {
			gluBuild2DMipmaps(target,GL_RGBA,width,height,GL_RGBA,GL_UNSIGNED_BYTE,data);
		}
	}
	free(data);
}

/*
 */
void Texture::enable() {
	glEnable(target);
}

/*
 */
void Texture::disable() {
	glDisable(target);
}

/*
 */
void Texture::bind() {
	glBindTexture(target,id);
}

/* image loaders
 */
unsigned char *Texture::load(const char *name,int *width,int *height) {
	char *ext = strrchr(name,'.');
	if(!ext) {
		fprintf(stderr,"unknown file format");
		return NULL;
	}
	unsigned char *data = NULL;
	if(!strcmp(ext,".tga")) data = load_tga(name,width,height);
	else if(!strcmp(ext,".jpg")) data = load_jpeg(name,width,height);
	else if(!strcmp(ext,".dds")) data = load_dds(name,width,height);
	if(!data) fprintf(stderr,"error open \"%s\" file\n",name);
	return data;
}


/* load tga image
 * only for 24 and 32 bits per pixel
 */
unsigned char *Texture::load_tga(const char *name,int *width,int *height) {
	int i,j,k,l,w,h,components,size;
	unsigned char rep,*data,*buf,*ptr,info[18];
	FILE *file = fopen(name,"rb");
	if(!file) return NULL;
	fread(&info,1,18,file);
	w = info[12] + info[13] * 256;
	h = info[14] + info[15] * 256;
	switch(info[16]) {
		case 32: components = 4; break;
		case 24: components = 3; break;
		default: fclose(file); return NULL;
	}
	size = w * h * components;
	buf = (unsigned char*)malloc(size);
	data = (unsigned char*)malloc(w * h * 4);
	if(!data || !buf) {
		fclose(file);
		return NULL;
	}
	fseek(file,info[0],SEEK_CUR);
	switch(info[2]) {
		case 2: fread(buf,1,size,file); break;
        case 10:
			i = 0;
			ptr = buf;
			while(i < size) {
				fread(&rep,1,1,file);
				if(rep & 0x80) {
					rep ^= 0x80;
					fread(ptr,1,components,file);
					ptr += components;
					for(j = 0; j < rep * components; j++) {
						*ptr = *(ptr - components);
						ptr ++;
					}
					i += components * (rep + 1);
				} else {
					k = components * (rep + 1);
					fread(ptr,1,k,file);
					ptr += k;
					i += k;
				}
			}
			break;
		default:
			fclose(file);
			free(buf);
			free(data);
			return NULL;
	}
	for(i = 0, j = 0; i < size; i += components, j += 4) {
		data[j] = buf[i + 2];
		data[j + 1] = buf[i + 1];
		data[j + 2] = buf[i];
		if(components == 4) data[j + 3] = buf[i + 3];
		else data[j + 3] = 255;
	}
	if(!(info[17] & 0x20))
		for(j = 0, k = w * 4; j < h / 2; j ++)
			for(i = 0; i < w * 4; i ++) {
				l = data[j * k + i];
				data[j * k + i] = data[(h - j - 1) * k + i];
				data[(h - j - 1) * k + i] = l;
			}
	fclose(file);
	free(buf);
	*width = w;
	*height = h;
	return data;
}

/* save tga image
 */
int Texture::save_tga(const char *name,const unsigned char *data,int width,int height) {
	int i,j;
	unsigned char *buf;
	FILE *file = fopen(name,"wb");
	if(!file) return -1;
	buf = (unsigned char*)malloc(18 + width * height * 4);
	memset(buf,0,18);
	buf[2] = 2;
	buf[12] = width % 256;
	buf[13] = width / 256;
	buf[14] = height % 256;
	buf[15] = height / 256;
	buf[16] = 32;
	buf[17] = 0x28;
	memcpy(buf + 18,data,width * height * 4);
	for(i = 18; i < 18 + width * height * 4; i += 4) {
		j = buf[i];
		buf[i] = buf[i + 2];
		buf[i + 2] = j;
	}
	fwrite(buf,1,18 + width * height * 4,file);
	fclose(file);
	free(buf);
	return 0;
}

/* load jpeg image
 */
struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

static void my_error_exit(j_common_ptr cinfo) {
	my_error_ptr myerr = (my_error_ptr)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(myerr->setjmp_buffer,1);
}

unsigned char *Texture::load_jpeg(const char *name,int *width,int *height) {
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	FILE *file;
	JSAMPARRAY buffer;
	int row_stride;
	long cont;
	JSAMPLE *data_buffer;
	int i,j;
	unsigned char *data;
	file = fopen(name,"rb");
	if(!file) return NULL;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if(setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(file);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,file);
	jpeg_read_header(&cinfo,TRUE);
	jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * cinfo.output_components;
	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,JPOOL_IMAGE,row_stride,1);
	data_buffer = (JSAMPLE*)malloc(cinfo.image_width * cinfo.image_height * cinfo.output_components);
	cont = 0;
	while(cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo,buffer,1);
		memcpy(data_buffer + cinfo.image_width * cinfo.output_components * cont,buffer[0],row_stride);
		cont++;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	*width = cinfo.image_width;
	*height = cinfo.image_height;
	data = (unsigned char*)malloc(*width * *height * 4);
	switch(cinfo.output_components) {
		case 1:
			for(i = 0, j = 0; i < *width * *height; i++, j += 4) {
				data[j] = data[j + 1] = data[j + 2] = data_buffer[i];
				data[j + 3] = 255;
			}
			break;
		case 3:
			for(i = 0, j = 0; i < *width * *height * 3; i += 3, j += 4) {
				data[j] = data_buffer[i];
				data[j + 1] = data_buffer[i + 1];
				data[j + 2] = data_buffer[i + 2];
				data[j + 3] = 255;
			}
			break;
		default:
			free(data);
			free(data_buffer);
			return NULL;
	}
	free(data_buffer);
	fclose(file);
	return data;
}

/* save jpeg image
 */
int Texture::save_jpeg(const char *name,const unsigned char *data,int width,int height,int quality) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int i,j,row_stride;
	unsigned char *data_buffer;
	FILE *file = fopen(name,"wb");
	if(!file) return -1;
	data_buffer = (unsigned char*)malloc(width * height * 3);
	for(i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
		data_buffer[j + 0] = data[i + 0];
		data_buffer[j + 1] = data[i + 1];
		data_buffer[j + 2] = data[i + 2];
	}
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo,file);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo,quality,TRUE);
	jpeg_start_compress(&cinfo,TRUE);
	row_stride = width * 3;
	while(cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &data_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo,row_pointer,1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	free(data_buffer);
	fclose(file);
	return 0;
}

/* load DDS image
 */
struct dds_colorkey {
	unsigned int dwColorSpaceLowValue;
	unsigned int dwColorSpaceHighValue;
};

struct dds_header {
	unsigned int magic;
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	long lPitch;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwAlphaBitDepth;
	unsigned int dwReserved;
	void *lpSurface;
	dds_colorkey ddckCKDestOverlay;
	dds_colorkey ddckCKDestBlt;
	dds_colorkey ddckCKSrcOverlay;
	dds_colorkey ddckCKSrcBlt;
	unsigned int dwPFSize;
	unsigned int dwPFFlags;
	unsigned int dwFourCC;
	unsigned int dwRGBBitCount;
	unsigned int dwRBitMask;
	unsigned int dwGBitMask;
	unsigned int dwBBitMask;
	unsigned int dwRGBAlphaBitMask;
	unsigned int dwCaps;
	unsigned int dwCaps2;
	unsigned int dwCaps3;
	unsigned int dwVolumeDepth;
	unsigned int dwTextureStage;
};

struct dds_color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

enum {
	DDS_ERROR = -1,
	DDS_RGB,
	DDS_RGBA,
	DDS_DXT1,
	DDS_DXT2,
	DDS_DXT3,
	DDS_DXT4,
	DDS_DXT5
};

enum {
	DDPF_ALPHAPIXELS = 0x01,
	DDPF_FOURCC = 0x04,
	DDPF_RGB = 0x40,
	DDPF_RGBA = 0x41
};

unsigned char *Texture::load_dds(const char *name,int *width,int *height) {
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"error open DDS file \"%s\"\n",name);
		return NULL;
	}
	dds_header header;
	fread(&header,sizeof(dds_header),1,file);
	if(header.magic != ('D' | 'D' << 8 | 'S' << 16 | ' ' << 24)) {
		printf("error load DDS file \"%s\": wrong magic\n",name);
		fclose(file);
		return NULL;
	}
	*width = header.dwWidth;
	*height = header.dwHeight;
	int format = DDS_ERROR;
	if(header.dwPFFlags & DDPF_FOURCC) {
		switch(header.dwFourCC) {
			case ('D' | 'X' << 8 | 'T' << 16 | '1' << 24): format = DDS_DXT1; break;
			case ('D' | 'X' << 8 | 'T' << 16 | '2' << 24): format = DDS_DXT2; break;
			case ('D' | 'X' << 8 | 'T' << 16 | '3' << 24): format = DDS_DXT3; break;
			case ('D' | 'X' << 8 | 'T' << 16 | '4' << 24): format = DDS_DXT4; break;
			case ('D' | 'X' << 8 | 'T' << 16 | '5' << 24): format = DDS_DXT5; break;
		}
	}
	else if(header.dwPFFlags == DDPF_RGB && header.dwRGBBitCount == 24) format = DDS_RGB;
	else if(header.dwPFFlags == DDPF_RGBA && header.dwRGBBitCount == 32) format = DDS_RGBA;
	if(format == DDS_ERROR) {
		fprintf(stderr,"error load DDS file \"%s\": unknown format 0x%x RGB %d\n",name,header.dwPFFlags,header.dwRGBBitCount);
		fclose(file);
		return NULL;
	}
	if(format == DDS_DXT2 || format == DDS_DXT4) {
		fprintf(stderr,"error load DDS file \"%s\": DXT2 or DXT4 not supported\n",name);
		fclose(file);
		return NULL;
	}
	unsigned char *data = (unsigned char*)malloc(*width * *height * 4);
	if(format == DDS_RGB) {
		unsigned char *buf = new unsigned char[*width * *height * 3];
		fread(buf,*width * *height * 3,1,file);
		unsigned char *src = buf;
		unsigned char *dest = data;
		for(int y = 0; y < *height; y++) {
			for(int x = 0; x < *width; x++) {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = 255;
			}
		}
		delete buf;
	} else if(format == DDS_RGBA) {
		unsigned char *buf = new unsigned char[*width * *height * 4];
		fread(buf,*width * *height * 4,1,file);
		unsigned char *src = buf;
		unsigned char *dest = data;
		for(int y = 0; y < *height; y++) {
			for(int x = 0; x < *width; x++) {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
			}
		}
		delete buf;
	} else {
		unsigned char *buf = new unsigned char[*width * *height];
		unsigned char *src = buf;
		fread(buf,*width * *height,1,file);
		for(int y = 0; y < *height; y += 4) {
			for(int x = 0; x < *width; x += 4) {
				unsigned long long alpha = 0;
				unsigned int a0 = 0;
				unsigned int a1 = 0;
				if(format == DDS_DXT3) {
					alpha = *(unsigned long long*)src;
					src += 8;
				} else if(format == DDS_DXT5) {
					alpha=  (*(unsigned long long*)src) >> 16;
					a0 = src[0];
					a1 = src[1];
					src += 8;
				}
				unsigned int c0 = *(unsigned short*)(src + 0);
				unsigned int c1 = *(unsigned short*)(src + 2);
				src += 4;
				dds_color color[4];
				color[0].r = ((c0 >> 11) & 0x1f) << 3;
				color[0].g = ((c0 >> 5) & 0x3f) << 2;
				color[0].b = (c0 & 0x1f) << 3;
				color[1].r = ((c1 >> 11) & 0x1f) << 3;
				color[1].g = ((c1 >> 5) & 0x3f) << 2;
				color[1].b = (c1 & 0x1f) << 3;
				if(c0 > c1) {
					color[2].r = (color[0].r * 2 + color[1].r) / 3;
					color[2].g = (color[0].g * 2 + color[1].g) / 3;
					color[2].b = (color[0].b * 2 + color[1].b) / 3;
					color[3].r = (color[0].r + color[1].r * 2) / 3;
					color[3].g = (color[0].g + color[1].g * 2) / 3;
					color[3].b = (color[0].b + color[1].b * 2) / 3;
				} else {
					color[2].r = (color[0].r + color[1].r) / 2;
					color[2].g = (color[0].g + color[1].g) / 2;
					color[2].b = (color[0].b + color[1].b) / 2;
					color[3].r = 0;
					color[3].g = 0;
					color[3].b = 0;
				}
				for(int i = 0; i < 4; i++) {
					unsigned int index = *src++;
					unsigned char *dest = data + (*width * (y + i) + x) * 4;
					for(int j = 0; j < 4; j++) {
						*dest++ = color[index & 0x03].r;
						*dest++ = color[index & 0x03].g;
						*dest++ = color[index & 0x03].b;
						if(format == DDS_DXT1) {
							*dest++ = ((index & 0x03) == 3 && c0 <= c1) ? 0 : 255;
						} else if(format == DDS_DXT3) {
							*dest++ = (alpha & 0x0f) << 4;
							alpha >>= 4;
						} else if(format == DDS_DXT5) {
							unsigned int a = alpha & 0x07;
							if(a == 0) *dest++ = a0;
							else if(a == 1) *dest++ = a1;
							else if(a0 > a1) *dest++ = ((8 - a) * a0 + (a - 1) * a1) / 7;
							else if(a > 5) *dest++ = (a == 6) ? 0 : 255;
							else *dest++ = ((6 - a) * a0 + (a - 1) * a1) / 5;
							alpha >>= 3;
						} else *dest++ = 255;
						index >>= 2;
					}
				}
			}
		}
		delete buf;
	}
	fclose(file);
	return data;
}
