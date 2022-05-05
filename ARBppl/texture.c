/*	texture for opengl
 *	support tga and jpeg images
 *
 *		written by Alexander Zaprjagaev
 *		frustum@public.tsu.ru
 */

#include "texture.h"

/* load tga image
 * only for 24 and 32 bits per pixel
 */
unsigned char *texture_load_tga(char *name,int *width,int *height) {
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
	buf = malloc(size);
	data = malloc(w * h * 4);
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
int texture_save_tga(char *name,unsigned char *data,int width,int height) {
	int i,j;
	unsigned char *buf;
	FILE *file = fopen(name,"wb");
	if(!file) return -1;
	buf = malloc(18 + width * height * 4);
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

/* for loading jpeg
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

/* load jpeg image
 */
unsigned char *texture_load_jpeg(char *name,int *width,int *height) {
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
	data_buffer = malloc(cinfo.image_width * cinfo.image_height * cinfo.output_components);
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
	data = malloc(*width * *height * 4);
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
int texture_save_jpeg(char *name,unsigned char *data,int width,int height,int quality) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int i,j,row_stride;
	unsigned char *data_buffer;
	FILE *file = fopen(name,"wb");
	if(!file) return -1;
	data_buffer = malloc(width * height * 3);
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

/* load openGL texture from file
 */
int texture_load(char *name,int mode) {
	int id,width,height;
	unsigned char *data = NULL;
	char *ext = strrchr(name,'.');
	if(!ext) {
		fprintf(stderr,"unknown file format\n");
		return -1;
	}
	if(!strcasecmp(ext,".tga")) data = texture_load_tga(name,&width,&height);
	else if(!strcasecmp(ext,".jpg")) data = texture_load_jpeg(name,&width,&height);
	if(!data) {
		fprintf(stderr,"error open \"%s\" file\n",name);
		return -1;
	}
	glGenTextures(1,&id);
	glBindTexture(GL_TEXTURE_2D,id);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	if(mode & TEXTURE_TRILINEAR) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	}
	if(mode & TEXTURE_CLAMP) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}
	if(mode & TEXTURE_MIPMAP_SGIS) {
		glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	} else {
		gluBuild2DMipmaps(GL_TEXTURE_2D,4,width,height,GL_RGBA,GL_UNSIGNED_BYTE,data);
	}
	free(data);
	return id;
}

/* delete openGL texture
 */
void texture_free(int id) {
	glDeleteTextures(1,&id);
}
