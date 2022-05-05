/*	OpenGL pixel buffer
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.tomsk.ru
 *			http://frustum.tomsk.ru
 */

#ifndef __PBUFFER_H__
#define __PBUFFER_H__

struct PBuffer_data;

class PBuffer {
public:
	PBuffer(int width,int height,int float_buffer = 0);
	~PBuffer();
	
	int enable();
	int disable();
	
	int width;
	int height;
	
protected:
	
	struct PBuffer_data *data;
	
};

#endif /* __PBUFFER_H__ */
