/* OpenGL Render Target
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

#ifndef __RENDERTARGET_H__
#define __RENDERTARGET_H__

class Texture;
struct RenderTargetData;

/*
 */
class RenderTarget {
	
	public:
		
		enum {
			FLOAT_16 = 1 << 0,
			FLOAT_32 = 1 << 1,
			MULTISAMPLE_2 = 1 << 2,
			MULTISAMPLE_4 = 1 << 3,
		};
		
		RenderTarget(int width,int height,int flags = 0);
		virtual ~RenderTarget();
		
		void setTarget(Texture *texture);
		void setFace(int face);
		void flush();
		
		void enable();
		void disable();
		
		int width;
		int height;
		int flags;
		
	protected:
		
		RenderTargetData *data;
		
		Texture *target;
		int face;
};

#endif /* __RENDERTARGET_H__ */
