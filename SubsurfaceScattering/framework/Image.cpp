/* Image
 *
 * Copyright (C) 2003-2005, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <setjmp.h>
extern "C" {
	#include <jpeglib.h>
	#include <jerror.h>
}

#ifdef _WIN32
#pragma comment (lib,"libjpeg.lib")
#pragma comment (lib,"libpng.lib")
#pragma comment (lib,"zlib.lib")
#endif

#include "Image.h"

/*****************************************************************************/
/*                                                                           */
/* Image                                                                     */
/*                                                                           */
/*****************************************************************************/

/*
 */
Image::Image() {
	width = 0;
	height = 0;
	data = new unsigned char[4];
}

Image::Image(const char *name) {
	data = NULL;
	load(name);
}

Image::Image(int width,int height) {
	this->width = width;
	this->height = height;
	data = new unsigned char[width * height * 4];
	memset(data,0,sizeof(unsigned char) * 4);
}

Image::~Image() {
	delete [] data;
}

/*
 */
int Image::load(const char *name) {
	if(data) delete [] data;
	data = load(name,width,height);
	if(data == NULL) {
		width = 0;
		height = 0;
		data = new unsigned char[4];
		return 0;
	}
	return 1;
}

/*
 */
int Image::load(char **src) {
	if(data) delete [] data;
	data = load_xpm(src,width,height);
	if(data == NULL) {
		width = 0;
		height = 0;
		data = new unsigned char[4];
		return 0;
	}
	return 1;
}

/*
 */
int Image::save(const char *name) {
	return save(name,data,width,height);
}

/*
 */
Image::Pixel Image::get(int x,int y) {
	Pixel p;
	if(width == 0 || height == 0) {
		p.r = 0;
		p.g = 0;
		p.b = 0;
		p.a = 0;
		return p;
	}
	x = x % width;
	y = y % height;
	if(x < 0) x += width;
	if(y < 0) y += height;
	unsigned char *d = &data[(width * y + x) << 2];
	p.r = *d++;
	p.g = *d++;
	p.b = *d++;
	p.a = *d++;
	return p;
}

/*
 */
Image::Pixel Image::get(float s,float t) {
	Pixel p;
	float x = (float)width * s - 0.5f;
	float y = (float)height * t - 0.5f;
	int x0 = (int)floor(x);
	int y0 = (int)floor(y);
	int x1 = (int)ceil(x);
	int y1 = (int)ceil(y);
	Pixel p00 = get(x0,y0);
	Pixel p10 = get(x1,y0);
	Pixel p01 = get(x0,y1);
	Pixel p11 = get(x1,y1);
	float k00 = (1.0f - (x - x0)) * (1.0f - (y - y0));
	float k10 = (x - x0) * (1.0f - (y - y0));
	float k01 = (1.0f - (x - x0)) * (y - y0);
	float k11 = (x - x0) * (y - y0);
	p.r = (unsigned char)((float)p00.r * k00 + (float)p01.r * k01 + (float)p10.r * k10 + (float)p11.r * k11);
	p.g = (unsigned char)((float)p00.g * k00 + (float)p01.g * k01 + (float)p10.g * k10 + (float)p11.g * k11);
	p.b = (unsigned char)((float)p00.b * k00 + (float)p01.b * k01 + (float)p10.b * k10 + (float)p11.b * k11);
	p.a = (unsigned char)((float)p00.a * k00 + (float)p01.a * k01 + (float)p10.a * k10 + (float)p11.a * k11);
	return p;
}

/*
 */
void Image::put(int x,int y,const Pixel &p) {
	if(width == 0 || height == 0) return;
	unsigned char *d = &data[(width * (y % height) + (x % width)) << 2];
	*d++ = p.r;
	*d++ = p.g;
	*d++ = p.b;
	*d++ = p.a;
}

/*****************************************************************************/
/*                                                                           */
/* 2D Images                                                                 */
/*                                                                           */
/*****************************************************************************/

/*
 */
unsigned char *Image::load(const char *name,int &width,int &height) {
	if(name == NULL) {
		fprintf(stderr,"Image::load(): name is NULL\n");
		return 0;
	}
	if(strstr(name,".tga") || strstr(name,".TGA")) return load_tga(name,width,height);
	if(strstr(name,".jpg") || strstr(name,".JPG")) return load_jpeg(name,width,height);
	if(strstr(name,".png") || strstr(name,".PNG")) return load_png(name,width,height);
	if(strstr(name,".dds") || strstr(name,".DDS")) return load_dds(name,width,height);
	if(strstr(name,".xpm") || strstr(name,".XPM")) return load_xpm(name,width,height);
	fprintf(stderr,"Image::load(): unknown format of \"%s\" file\n",name);
	return NULL;
}

/*
 */
int Image::save(const char *name,const unsigned char *data,int width,int height) {
	if(name == NULL) {
		fprintf(stderr,"Image::save(): name is NULL\n");
		return 0;
	}
	if(strstr(name,".tga") || strstr(name,".TGA")) return save_tga(name,data,width,height);
	if(strstr(name,".jpg") || strstr(name,".JPG")) return save_jpeg(name,data,width,height,85);
	if(strstr(name,".png") || strstr(name,".PNG")) return save_png(name,data,width,height);
	fprintf(stderr,"Image::save(): unknown format of \"%s\" file\n",name);
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* TGA Images                                                                */
/*                                                                           */
/*****************************************************************************/

/*
 */
#pragma pack(1)

struct tga_header {
	unsigned char id_length;
	unsigned char colormap_type;
	unsigned char image_type;
	unsigned short colormap_index;
	unsigned short colormap_length;
	unsigned char colormap_size;
	unsigned short x_orign;
	unsigned short y_orign;
	unsigned short width;
	unsigned short height;
	unsigned char pixel_size;
	unsigned char attributes;
};

#pragma pack(4)

/* load tga images
 */
unsigned char *Image::load_tga(const char *name,int &width,int &height) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_tga(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	tga_header header;
	fread(&header,sizeof(header),1,file);
	width = header.width;
	height = header.height;
	
	// skip comment
	fseek(file,header.id_length,SEEK_CUR);
	
	// colormap
	unsigned char *colormap = NULL;
	if(header.colormap_length > 0) {
		int size = header.colormap_length * header.colormap_size / 8;
		colormap = new unsigned char[size];
		fread(colormap,size,1,file);
	}
	
	// image
	unsigned char *data = new unsigned char[width * height * 4];
	
	// uncompressed colormapped and uncompressed monochrome image
	if(header.image_type == 1 || header.image_type == 3) {
		int size = width * height;
		unsigned char *buf = new unsigned char[size];
		fread(buf,sizeof(unsigned char),size,file);
		// colormapped image
		if(header.image_type == 1) {
			for(int i = 0, j = 0; i < size; i++, j += 4) {
				if(header.colormap_size == 8) {
					data[j + 0] = colormap[buf[i]];
					data[j + 1] = colormap[buf[i]];
					data[j + 2] = colormap[buf[i]];
					data[j + 3] = 255;
				} else if(header.colormap_size == 24) {
					data[j + 0] = colormap[buf[i] * 3 + 2];
					data[j + 1] = colormap[buf[i] * 3 + 1];
					data[j + 2] = colormap[buf[i] * 3 + 0];
					data[j + 3] = 255;
				} else if(header.colormap_size == 32) {
					data[j + 0] = colormap[buf[i] * 4 + 3];
					data[j + 1] = colormap[buf[i] * 4 + 2];
					data[j + 2] = colormap[buf[i] * 4 + 1];
					data[j + 3] = colormap[buf[i] * 4 + 0];
				}
			}
		}
		// monochrome image
		else {
			for(int i = 0, j = 0; i < size; i++, j += 4) {
				data[j + 0] = buf[i];
				data[j + 1] = buf[i];
				data[j + 2] = buf[i];
				data[j + 3] = 255;
			}
		}
		delete [] buf;
	}
	// uncompressed and compressed RGB/RGBA image
	else if(header.image_type == 2 || header.image_type == 10) {
		int components = 0;
		if(header.pixel_size == 24) components = 3;
		else if(header.pixel_size == 32) components = 4;
		else {
			fprintf(stderr,"Image::load_tga(): only 24 and 32 bpp images are suported\n");
			fclose(file);
			if(colormap) delete [] colormap;
			delete [] data;
			return NULL;
		}
		int size = width * height * components;
		unsigned char *buf = new unsigned char[size];
		// uncompressed
		if(header.image_type == 2) {
			fread(buf,sizeof(unsigned char),size,file);
		}
		// RLE compressed
		else {
			unsigned char *ptr = buf;
			for(int i = 0; i < size;) {
				unsigned char rep = fgetc(file);
				if(rep & 0x80) {
					rep ^= 0x80;
					fread(ptr,sizeof(unsigned char),components,file);
					ptr += components;
					for(int j = 0; j < rep * components; j++) {
						*ptr = *(ptr - components);
						ptr++;
					}
					i += components * (rep + 1);
				} else {
					int j = components * (rep + 1);
					fread(ptr,sizeof(unsigned char),j,file);
					ptr += j;
					i += j;
				}
			}
		}
		// convert to RGBA
		for(int i = 0, j = 0; i < size; i += components, j += 4) {
			data[j] = buf[i + 2];
			data[j + 1] = buf[i + 1];
			data[j + 2] = buf[i];
			if(components == 3) data[j + 3] = 255;
			else data[j + 3] = buf[i + 3];
		}
		delete [] buf;
	}
	else {
		fprintf(stderr,"Image::load_tga(): unsupported image type %d\n",header.image_type);
		fclose(file);
		if(colormap) delete [] colormap;
		delete [] data;
		return NULL;
	}
	
	// flip image
	if((header.attributes & 0x20) == 0) {
		for(int y = 0; y < height / 2; y++) {
			int c;
			int *s = (int*)&data[width * y * 4];
			int *d = (int*)&data[(height - y - 1) * width * 4];
			for(int x = 0; x < width; x++) {
				c = *d;
				*d++ = *s;
				*s++ = c;
			}
		}
	}
	
	if(colormap) delete [] colormap;
	fclose(file);
	
	return data;
}

/* save tga images
 */
int Image::save_tga(const char *name,const unsigned char *data,int width,int height) {
	
	FILE *file = fopen(name,"wb");
	if(file == NULL) {
		fprintf(stderr,"Image::save_tga(): error create \"%s\" file\n",name);
		return 0;
	}
	
	tga_header header;
	memset(&header,0,sizeof(header));
	header.image_type = 2;
	header.width = width;
	header.height = height;
	header.pixel_size = 32;
	header.attributes = 0x28;
	fwrite(&header,sizeof(header),1,file);
	
	// rgba->bgra
	int size = width * height * 4;
	unsigned char *buf = new unsigned char[size];
	for(int i = 0; i < size; i += 4) {
		buf[i + 0] = data[i + 2];
		buf[i + 1] = data[i + 1];
		buf[i + 2] = data[i + 0];
		buf[i + 3] = data[i + 3];
	}
	
	fwrite(buf,sizeof(unsigned char),size,file);
	
	delete [] buf;
	fclose(file);
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* JPEG Images                                                               */
/*                                                                           */
/*****************************************************************************/

/* load jpeg images
 */
struct my_error_mgr {
	jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr cinfo) {
	my_error_mgr *my_error = (my_error_mgr*)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(my_error->setjmp_buffer,1);
}

/*
 */
struct my_source_mgr {
	jpeg_source_mgr pub;
	FILE *file;
	JOCTET buffer[4096];
	int start_of_file;
};

static void jpeg_init_source(j_decompress_ptr cinfo) {
	my_source_mgr *src = (my_source_mgr*)cinfo->src;
	src->start_of_file = 1;
}

static boolean jpeg_fill_input_buffer(j_decompress_ptr cinfo) {
	my_source_mgr *src = (my_source_mgr*)cinfo->src;
	size_t bytes = fread(src->buffer,1,sizeof(src->buffer),src->file);
	if(bytes <= 0) {
		if(src->start_of_file) ERREXIT(cinfo,JERR_INPUT_EMPTY);
		WARNMS(cinfo,JWRN_JPEG_EOF);
		src->buffer[0] = (JOCTET)0xff;
		src->buffer[1] = (JOCTET)JPEG_EOI;
		bytes = 2;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = bytes;
	src->start_of_file = 0;
	return true;
}

static void jpeg_skip_input_data(j_decompress_ptr cinfo,long num_bytes) {
	my_source_mgr *src = (my_source_mgr*)cinfo->src;
	if(num_bytes > 0) {
		while(num_bytes > (long)src->pub.bytes_in_buffer) {
			num_bytes -= (long)src->pub.bytes_in_buffer;
			jpeg_fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t)num_bytes;
		src->pub.bytes_in_buffer -= (size_t)num_bytes;
	}
}

static void jpeg_term_source(j_decompress_ptr cinfo) {
	
}

static void jpeg_file_source(j_decompress_ptr cinfo,FILE *file) {
	my_source_mgr *src = NULL;
	if(cinfo->src == NULL) {
		cinfo->src = (jpeg_source_mgr*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo,JPOOL_PERMANENT,sizeof(my_source_mgr));
	}
	src = (my_source_mgr*)cinfo->src;
	src->pub.init_source = jpeg_init_source;
	src->pub.fill_input_buffer = jpeg_fill_input_buffer;
	src->pub.skip_input_data = jpeg_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = jpeg_term_source;
	src->file = file;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}

/* load jpeg image
 */
unsigned char *Image::load_jpeg(const char *name,int &width,int &height) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_jpeg(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	jpeg_decompress_struct cinfo;
	my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if(setjmp(jerr.setjmp_buffer)) {
		fprintf(stderr,"Image::load_jpeg(): error load \"%s\" file\n",name);
		jpeg_destroy_decompress(&cinfo);
		fclose(file);
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	jpeg_file_source(&cinfo,file);
	jpeg_read_header(&cinfo,true);
	jpeg_start_decompress(&cinfo);
	
	int count = 0;
	int row_stride = cinfo.output_width * cinfo.output_components;
	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,JPOOL_IMAGE,row_stride,1);
	JSAMPLE *data_buffer = new JSAMPLE[cinfo.image_width * cinfo.image_height * cinfo.output_components];
	while(cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo,buffer,1);
		memcpy(data_buffer + cinfo.image_width * cinfo.output_components * count,buffer[0],row_stride);
		count++;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	width = cinfo.image_width;
	height = cinfo.image_height;
	unsigned char *data = new unsigned char[width * height * 4];
	if(cinfo.output_components == 1) {
		for(int i = 0, j = 0; i < width * height; i++, j += 4) {
			data[j] = data[j + 1] = data[j + 2] = data_buffer[i];
			data[j + 3] = 255;
		}
	} else if(cinfo.output_components == 3) {
		for(int i = 0, j = 0; i < width * height * 3; i += 3, j += 4) {
			data[j] = data_buffer[i];
			data[j + 1] = data_buffer[i + 1];
			data[j + 2] = data_buffer[i + 2];
			data[j + 3] = 255;
		}
	} else {
		delete [] data;
		delete [] data_buffer;
		return NULL;
	}
	
	delete [] data_buffer;
	fclose(file);
	
	return data;
}

/*
 */
struct my_destination_mgr {
	jpeg_destination_mgr pub;
	FILE *file;
	JOCTET buffer[4096];
};

static void jpeg_init_destination(j_compress_ptr cinfo) {
	my_destination_mgr *dest = (my_destination_mgr*)cinfo->dest;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = sizeof(dest->buffer);
}

static boolean jpeg_empty_output_buffer(j_compress_ptr cinfo) {
	my_destination_mgr *dest = (my_destination_mgr*)cinfo->dest;
	size_t bytes = fwrite(dest->buffer,1,sizeof(dest->buffer),dest->file);
	if(bytes != sizeof(dest->buffer)) {
		ERREXIT(cinfo,JERR_FILE_WRITE);
	}
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = sizeof(dest->buffer);
	return true;
}

static void jpeg_term_destination(j_compress_ptr cinfo) {
	my_destination_mgr *dest = (my_destination_mgr*)cinfo->dest;
	size_t bytes = sizeof(dest->buffer) - dest->pub.free_in_buffer;
	if(bytes > 0) {
		if(fwrite(dest->buffer,1,bytes,dest->file) != bytes) {
			ERREXIT(cinfo,JERR_FILE_WRITE);
		}
	}
}

static void jpeg_file_destination(j_compress_ptr cinfo,FILE *file) {
	my_destination_mgr *dest = NULL;
	if(cinfo->dest == NULL) {
		cinfo->dest = (jpeg_destination_mgr*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo,JPOOL_PERMANENT,sizeof(my_destination_mgr));
	}
	dest = (my_destination_mgr*)cinfo->dest;
	dest->pub.init_destination = jpeg_init_destination;
	dest->pub.empty_output_buffer = jpeg_empty_output_buffer;
	dest->pub.term_destination = jpeg_term_destination;
	dest->file = file;
}

/* save jpeg images
 */
int Image::save_jpeg(const char *name,const unsigned char *data,int width,int height,int quality) {
	
	FILE *file = fopen(name,"wb");
	if(file == NULL) {
		fprintf(stderr,"Image::save_jpeg(): error create \"%s\" file\n",name);
		return 0;
	}
	
	unsigned char *data_buffer = new unsigned char[width * height * 3];
	for(int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
		data_buffer[j + 0] = data[i + 0];
		data_buffer[j + 1] = data[i + 1];
		data_buffer[j + 2] = data[i + 2];
	}
	
	jpeg_compress_struct cinfo;
	my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if(setjmp(jerr.setjmp_buffer)) {
		fprintf(stderr,"Image::save_jpeg(): error save \"%s\" file\n",name);
		jpeg_destroy_compress(&cinfo);
		fclose(file);
		return 0;
	}
	jpeg_create_compress(&cinfo);
	jpeg_file_destination(&cinfo,file);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo,quality,true);
	
	jpeg_start_compress(&cinfo,true);
	int row_stride = width * 3;
	while(cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW row_pointer;
		row_pointer = &data_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo,&row_pointer,1);
	}
	
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	
	delete [] data_buffer;
	fclose(file);
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* PNG Images                                                                */
/*                                                                           */
/*****************************************************************************/

/*
 */
static void png_read_function(png_structp png_ptr,png_bytep ptr,png_size_t size) {
	fread(ptr,sizeof(png_byte),size,(FILE*)png_get_io_ptr(png_ptr));
}

/* load png images
 */
unsigned char *Image::load_png(const char *name,int &width,int &height) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_png(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	png_byte sig[8];
	fread(sig,sizeof(png_byte),8,file);
	if(!png_check_sig(sig,8)) {
		fprintf(stderr,"Image::load_png(): wrong signature in \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if(png_ptr == NULL) {
		fclose(file);
		return NULL;
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr,0,0);
		fclose(file);
		return NULL;
	}
	
	png_set_read_fn(png_ptr,file,png_read_function);
	png_set_sig_bytes(png_ptr,8);
	png_read_info(png_ptr,info_ptr);
	unsigned long w,h;
	int bit_depth,color_type;
	png_get_IHDR(png_ptr,info_ptr,&w,&h,&bit_depth,&color_type,0,0,0);
	if(bit_depth == 16) png_set_strip_16(png_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_expand(png_ptr);
	if(bit_depth < 8) png_set_expand(png_ptr);
	if(png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) png_set_expand(png_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
	double gamma;
	if(png_get_gAMA(png_ptr,info_ptr,&gamma)) png_set_gamma(png_ptr,(double)2.2,gamma);
	png_read_update_info(png_ptr,info_ptr);
	png_get_IHDR(png_ptr,info_ptr,&w,&h,&bit_depth,&color_type,0,0,0);
	png_uint_32 row_bytes = png_get_rowbytes(png_ptr,info_ptr);
	png_uint_32 channels = png_get_channels(png_ptr,info_ptr);
	png_byte *img = new png_byte[row_bytes * h];
	png_byte **row = new png_byte*[h];
	for(int i = 0; i < (int)h; i++) {
		row[i] = img + row_bytes * i;
	}
	png_read_image(png_ptr,row);
	png_read_end(png_ptr,NULL);
	png_destroy_read_struct(&png_ptr,0,0);
	delete [] row;
	
	width = w;
	height = h;
	unsigned char *data = new unsigned char[width * height * 4];
	unsigned char *ptr = data;
	for(int i = 0; i < height; i++) {
		for(int j = 0; j < width; j++) {
			int k = row_bytes * i + j * channels;
			*ptr++ = img[k + 0];
			*ptr++ = img[k + 1];
			*ptr++ = img[k + 2];
			if(channels == 4) *ptr++ = img[k + 3];
			else *ptr++ = 255;
		}
	}
	
	delete [] img;
	fclose(file);
	
	return data;
}

/*
 */
static void png_write_function(png_structp png_ptr,png_bytep ptr,png_size_t size) {
	fwrite(ptr,sizeof(png_byte),size,(FILE*)png_get_io_ptr(png_ptr));
}

/*
 */
static void png_flush_function(png_structp png_ptr) {
	fflush((FILE*)png_get_io_ptr(png_ptr));
}

/* save png images
 */
int Image::save_png(const char *name,const unsigned char *data,int width,int height) {
	
	FILE *file = fopen(name,"wb");
	if(file == NULL) {
		fprintf(stderr,"Image::save_png(): error create \"%s\" file\n",name);
		return 0;
	}	
	
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if(png_ptr == NULL) {
		return 0;
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr,0);
		return 0;
	}
	
	png_set_write_fn(png_ptr,file,png_write_function,png_flush_function);
	png_set_IHDR(png_ptr,info_ptr,width,height,8,PNG_COLOR_TYPE_RGB_ALPHA,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr,info_ptr);
	png_byte **row = new png_byte*[height];
	for(int i = 0; i < height; i++) {
		row[i] = (png_byte*)data + width * 4 * i;
	}
	png_write_rows(png_ptr,row,height);
	png_write_end(png_ptr,info_ptr);
	png_destroy_write_struct(&png_ptr,&info_ptr);
	delete [] row;
	
	fclose(file);
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* DDS Images                                                                */
/*                                                                           */
/*****************************************************************************/

/*
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

/*
 */
unsigned char *Image::load_dds(const char *name,int &width,int &height) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_dds(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	dds_header header;
	fread(&header,sizeof(dds_header),1,file);
	if(header.magic != ('D' | 'D' << 8 | 'S' << 16 | ' ' << 24)) {
		fprintf(stderr,"Image::load_dds(): wrong magic in \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	width = header.dwWidth;
	height = header.dwHeight;
	int format = DDS_ERROR;
	
	if(header.dwPFFlags & DDPF_FOURCC) {
		if(header.dwFourCC == ('D' | ('X' << 8) | ('T' << 16) | ('1' << 24))) format = DDS_DXT1;
		else if(header.dwFourCC == ('D' | ('X' << 8) | ('T' << 16) | ('2' << 24))) format = DDS_DXT2;
		else if(header.dwFourCC == ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24))) format = DDS_DXT3;
		else if(header.dwFourCC == ('D' | ('X' << 8) | ('T' << 16) | ('4' << 24))) format = DDS_DXT4;
		else if(header.dwFourCC == ('D' | ('X' << 8) | ('T' << 16) | ('5' << 24))) format = DDS_DXT5;
	}
	else if(header.dwPFFlags == DDPF_RGB && header.dwRGBBitCount == 24) format = DDS_RGB;
	else if(header.dwPFFlags == DDPF_RGBA && header.dwRGBBitCount == 32) format = DDS_RGBA;
	
	if(format == DDS_ERROR) {
		fprintf(stderr,"Image::load_dds(): unknown format of \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	if(format == DDS_DXT2 || format == DDS_DXT4) {
		fprintf(stderr,"Image::load_dds(): DXT2 or DXT4 is not supported in \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	unsigned char *data = new unsigned char[width * height * 4];
	if(format == DDS_RGB) {
		unsigned char *buf = new unsigned char[width * height * 3];
		fread(buf,sizeof(unsigned char),width * height * 3,file);
		unsigned char *src = buf;
		unsigned char *dest = data;
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = 255;
			}
		}
		delete [] buf;
	}
	else if(format == DDS_RGBA) {
		unsigned char *buf = new unsigned char[width * height * 4];
		fread(buf,sizeof(unsigned char),width * height * 4,file);
		unsigned char *src = buf;
		unsigned char *dest = data;
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
			}
		}
		delete [] buf;
	}
	else {
		unsigned char *buf = new unsigned char[width * height];
		unsigned char *src = buf;
		fread(buf,sizeof(unsigned char),width * height,file);
		for(int y = 0; y < height; y += 4) {
			for(int x = 0; x < width; x += 4) {
				unsigned long long alpha = 0;
				unsigned int a0 = 0;
				unsigned int a1 = 0;
				if(format == DDS_DXT3) {
					alpha = *(unsigned long long*)src;
					src += 8;
				} else if(format == DDS_DXT5) {
					alpha = (*(unsigned long long*)src) >> 16;
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
					unsigned char *dest = data + (width * (y + i) + x) * 4;
					for(int j = 0; j < 4; j++) {
						*dest++ = color[index & 0x03].r;
						*dest++ = color[index & 0x03].g;
						*dest++ = color[index & 0x03].b;
						if(format == DDS_DXT1) {
							*dest++ = ((index & 0x03) == 3 && c0 <= c1) ? 0 : 255;
						} else if(format == DDS_DXT3) {
							*dest++ = (unsigned char)((alpha & 0x0f) << 4);
							alpha >>= 4;
						} else if(format == DDS_DXT5) {
							unsigned int a = (unsigned int)(alpha & 0x07);
							if(a == 0) *dest++ = a0;
							else if(a == 1) *dest++ = a1;
							else if(a0 > a1) *dest++ = ((8 - a) * a0 + (a - 1) * a1) / 7;
							else if(a > 5) *dest++ = (a == 6) ? 0 : 255;
							else *dest++ = ((6 - a) * a0 + (a - 1) * a1) / 5;
							alpha >>= 3;
						} else {
							*dest++ = 255;
						}
						index >>= 2;
					}
				}
			}
		}
		delete [] buf;
	}
	
	fclose(file);
	return data;
}

/*****************************************************************************/
/*                                                                           */
/* XPM Images                                                                */
/*                                                                           */
/*****************************************************************************/

/*
 */
unsigned char *Image::load_xpm(const char *name,int &width,int &height) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_xpm(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	char buf[1024];
	fread(buf,strlen("/* XPM */"),1,file);
	if(strncmp(buf,"/* XPM */",strlen("/* XPM */"))) {
		fprintf(stderr,"Image::load_xpm(): wrong header in \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	int num_lines = 0;
	int length = 0;
	int max_length = 0;
	while(1) {
		char c = fgetc(file);
		if(c == EOF) break;
		if(c == '"') {
			num_lines++;
			if(max_length < length) max_length = length;
			length = 0;
		} else {
			length++;
		}
	}
	
	num_lines = num_lines / 2 + 1;
	max_length += 1;
	
	char **src = new char*[num_lines];
	for(int i = 0; i < num_lines; i++) {
		src[i] = new char[max_length];
		memset(src[i],0,sizeof(char) * max_length);
	}
	
	fseek(file,0,SEEK_SET);
	
	char **dest = src;
	char *d = *dest++;
	int bracket = 0;
	while(1) {
		char c = fgetc(file);
		if(c == EOF) break;
		if(c == '"') {
			if(bracket == 0) {
				bracket = 1;
			} else if(bracket == 1) {
				bracket = 0;
				d = *dest++;
			}
		}
		else if(bracket == 1) {
			*d++ = c;
		}
	}
	
	unsigned char *data = load_xpm(src,width,height);
	
	for(int i = 0; i < num_lines; i++) {
		delete [] src[i];
	}
	delete [] src;
	
	fclose(file);
	return data;
}

/*
 */
unsigned char *Image::load_xpm(char **src,int &width,int &height) {
	
	int num_colors = 0;
	int chars_per_pixel = 0;
	if(sscanf(*src++,"%d %d %d %d",&width,&height,&num_colors,&chars_per_pixel) != 4) {
		fprintf(stderr,"Image::load_xpm(): wrong format \"%s\"\n",*src);
		return NULL;
	}
	
	struct Color {
		int key;
		unsigned char color[4];
	};
	Color *colors = new Color[num_colors];
	for(int i = 0; i < num_colors; i++) {
		char *s = *src++;
		int key = 0;
		for(int j = 0; j < chars_per_pixel; j++) {
			key = key * 128 + *s++;
		}
		colors[i].key = key;
		colors[i].color[0] = 0;
		colors[i].color[1] = 0;
		colors[i].color[2] = 0;
		colors[i].color[3] = 0;
		while(*s == ' ' || *s == '\t') s++;
		if(*s++ != 'c') {
			fprintf(stderr,"Image::load_xpm(): unknown color key '%s'\n",s);
			continue;
		}
		while(*s == ' ' || *s == '\t') s++;
		if(!strcmp(s,"None")) {
			
		} else if(*s == '#') {
			int r,g,b;
			sscanf(s,"#%2x%2x%2x",&r,&g,&b);
			colors[i].color[0] = r;
			colors[i].color[1] = g;
			colors[i].color[2] = b;
			colors[i].color[3] = 255;
		} else {
			fprintf(stderr,"Image::load_xpm(): unknown color \"%s\"\n",s);
		}
	}
	
	unsigned char *data = new unsigned char[width * height * 4];
	memset(data,0,sizeof(unsigned char) * width * height * 4);
	for(int y = 0; y < height; y++) {
		char *s = *src++;
		unsigned char *d = &data[width * y * 4];
		for(int x = 0; x < width; x++) {
			int key = 0;
			for(int j = 0; j < chars_per_pixel; j++) {
				key = key * 128 + *s++;
			}
			Color *c = NULL;
			for(int i = 0; i < num_colors; i++) {
				if(colors[i].key == key) {
					c = &colors[i];
					break;
				}
			}
			if(c != NULL) {
				*d++ = c->color[0];
				*d++ = c->color[1];
				*d++ = c->color[2];
				*d++ = c->color[3];
			}
		}
	}
	
	return data;
}

/*****************************************************************************/
/*                                                                           */
/* 2D Converters                                                             */
/*                                                                           */
/*****************************************************************************/

/*
 */
unsigned char *Image::rgba2luminance(unsigned char *data,int width,int height) {
	unsigned char *dest = new unsigned char[width * height];
	unsigned char *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = *s++;
		s += 3;
	}
	delete [] data;
	return dest;
}

unsigned char *Image::rgba2luminance_alpha(unsigned char *data,int width,int height) {
	unsigned char *dest = new unsigned char[width * height * 2];
	unsigned char *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = *s++;
		s += 2;
		*d++ = *s++;
	}
	delete [] data;
	return dest;
}

unsigned char *Image::rgba2rgb(unsigned char *data,int width,int height) {
	unsigned char *dest = new unsigned char[width * height * 3];
	unsigned char *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		s++;
	}
	delete [] data;
	return dest;
}

unsigned char *Image::rgba2rgba(unsigned char *data,int width,int height) {
	unsigned char *dest = new unsigned char[width * height * 4];
	unsigned char *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}
	delete [] data;
	return dest;
}

/*
 */
float *Image::rgba2luminance_float(unsigned char *data,int width,int height) {
	float *dest = new float[width * height];
	float *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = (float)(*s++) / 255.0f;
		s += 3;
	}
	delete [] data;
	return dest;
}

float *Image::rgba2luminance_alpha_float(unsigned char *data,int width,int height) {
	float *dest = new float[width * height * 2];
	float *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = (float)(*s++) / 255.0f;
		s += 2;
		*d++ = (float)(*s++) / 255.0f;
	}
	delete [] data;
	return dest;
}

float *Image::rgba2rgb_float(unsigned char *data,int width,int height) {
	float *dest = new float[width * height * 3];
	float *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = (float)(*s++) / 255.0f;
		*d++ = (float)(*s++) / 255.0f;
		*d++ = (float)(*s++) / 255.0f;
		s++;
	}
	delete [] data;
	return dest;
}

float *Image::rgba2rgba_float(unsigned char *data,int width,int height) {
	float *dest = new float[width * height * 4];
	float *d = dest;
	unsigned char *s = data;
	for(int i = 0; i < width * height; i++) {
		*d++ = (float)(*s++) / 255.0f;
		*d++ = (float)(*s++) / 255.0f;
		*d++ = (float)(*s++) / 255.0f;
		*d++ = (float)(*s++) / 255.0f;
	}
	delete [] data;
	return dest;
}

/*****************************************************************************/
/*                                                                           */
/* 3D Images                                                                 */
/*                                                                           */
/*****************************************************************************/

/*
 */
unsigned char *Image::load_3d(const char *name,int &width,int &height,int &depth) {
	
	FILE *file = fopen(name,"rb");
	if(file == NULL) {
		fprintf(stderr,"Image::load_3d(): can't open \"%s\" file\n",name);
		return NULL;
	}
	
	int magic;
	fread(&magic,sizeof(int),1,file);
	if(magic != ('3' | 'D' << 8 | 'T' << 16 | 'X' << 24)) {
		fprintf(stderr,"Image::load_3d(): wrong magic in \"%s\" file\n",name);
		fclose(file);
		return NULL;
	}
	
	fread(&width,sizeof(int),1,file);
	fread(&height,sizeof(int),1,file);
	fread(&depth,sizeof(int),1,file);
	
	int size = width * height * depth * 4;
	unsigned char *data = new unsigned char[size];
	fread(data,sizeof(unsigned char),size,file);
	
	fclose(file);
	return data;
}

/*
 */
int Image::save_3d(const char *name,const unsigned char *data,int width,int height,int depth) {
	
	FILE *file = fopen(name,"wb");
	if(file == NULL) {
		fprintf(stderr,"Image::save_3d(): error create \"%s\" file\n",name);
		return 0;
	}
	
	int magic = ('3' | 'D' << 8 | 'T' << 16 | 'X' << 24);
	fwrite(&magic,sizeof(int),1,file);
	fwrite(&width,sizeof(int),1,file);
	fwrite(&height,sizeof(int),1,file);
	fwrite(&depth,sizeof(int),1,file);
	
	int size = width * height * depth * 4;
	fwrite((void*)data,sizeof(unsigned char),size,file);
	
	fclose(file);
	return 1;
}
