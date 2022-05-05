/* Font
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

#ifndef __FONT_H__
#define __FONT_H__

#ifdef _WIN32
#include <windows.h>
#define vsnprintf _vsnprintf
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#define BLACK			"\033[30m"
#define RED				"\033[31m"
#define GREEN			"\033[32m"
#define BROWN			"\033[33m"
#define BLUE			"\033[34m"
#define MAGENTA			"\033[35m"
#define CYAN			"\033[36m"
#define WHITE			"\033[37m"
#define DEFAULT			"\033[0m"
#define UNDERLINE_ON	"\033[4m"
#define UNDERLINE_OFF	"\033[24m"

/*
 */
class Font {
	
	public:
		
		Font(const char *name);
		virtual ~Font();
		
		void enable(int width,int height);
		void disable();
		
		int getSize(char c = 0);
		void getSize(int font,const char *str,int &x,int &y);
		
		void puts(int x,int y,int font,const char *str);
		void printf(int x,int y,int font,const char *format,...);
		
	protected:
		
		GLuint texture_id;
		GLuint list_id;
		
		int step;
		int space[256][2];
		int width;
		int height;
		
		float modelview[16];
		float projection[16];
};

#endif /* __FONT_H__ */
