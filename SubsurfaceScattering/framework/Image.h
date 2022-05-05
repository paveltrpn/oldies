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

#ifndef __IMAGE_H__
#define __IMAGE_H__

/*
 */
class Image {
		
	public:
		
		Image();
		Image(const char *name);
		Image(int width,int height);
		virtual ~Image();
		
		struct Pixel {
			unsigned char r,g,b,a;
		};
		
		int getWidth() { return width; }
		int getHeight() { return height; }
		
		int load(const char *name);
		int load(char **src);
		
		int save(const char *name);
		
		Pixel get(int x,int y);
		Pixel get(float s,float t);
		void put(int x,int y,const Pixel &p);
		
		// 2D Images
		static unsigned char *load(const char *name,int &width,int &height);
		static int save(const char *name,const unsigned char *data,int width,int height);
	
		static unsigned char *load_tga(const char *name,int &width,int &height);
		static unsigned char *load_jpeg(const char *name,int &width,int &height);
		static unsigned char *load_png(const char *name,int &width,int &height);
		static unsigned char *load_dds(const char *name,int &width,int &height);
		static unsigned char *load_xpm(const char *name,int &width,int &height);
		static unsigned char *load_xpm(char **src,int &width,int &height);
		
		static int save_tga(const char *name,const unsigned char *data,int width,int height);
		static int save_jpeg(const char *name,const unsigned char *data,int width,int height,int quality);
		static int save_png(const char *name,const unsigned char *data,int width,int height);
		
		// Converters
		static unsigned char *rgba2luminance(unsigned char *data,int width,int height);
		static unsigned char *rgba2luminance_alpha(unsigned char *data,int width,int height);
		static unsigned char *rgba2rgb(unsigned char *data,int width,int height);
		static unsigned char *rgba2rgba(unsigned char *data,int width,int height);
		
		static float *rgba2luminance_float(unsigned char *data,int width,int height);
		static float *rgba2luminance_alpha_float(unsigned char *data,int width,int height);
		static float *rgba2rgb_float(unsigned char *data,int width,int height);
		static float *rgba2rgba_float(unsigned char *data,int width,int height);
		
		// 3D Images
		static unsigned char *load_3d(const char *name,int &width,int &height,int &depth);
		static int save_3d(const char *name,const unsigned char *data,int width,int height,int depth);
		
	private:
		
		int width;
		int height;
		unsigned char *data;
};

#endif /* __IMAGE_H__ */
