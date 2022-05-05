/* Vector
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

#ifndef __VECTOR_H__
#define __VECTOR_H__

/*
 */
template <class Type> class Vector {
		
	public:
		
		Vector() : length(0), capacity(0), data(NULL) {
			
		}
		Vector(const Vector &v) : length(v.length), capacity(v.length), data(NULL) {
			data = new Type[capacity];
			for(int i = 0; i < length; i++) {
				data[i] = v.data[i];
			}
		}
		Vector(int size) : length(0), capacity(size), data(NULL) {
			data = new Type[capacity];
		}
		Vector(Type *v,int size) : length(size), capacity(size), data(NULL) {
			data = new Type[capacity];
			for(int i = 0; i < length; i++) {
				data[i] = v[i];
			}
		}
		virtual ~Vector() {
			delete [] data;
		}
		
		Vector &operator=(const Vector &v) {
			if(v.length > capacity) {
				capacity = v.length;
				Type *new_data = new Type[capacity];
				delete [] data;
				data = new_data;
			}
			length = v.length;
			for(int i = 0; i < length; i++) {
				data[i] = v.data[i];
			}
			return *this;
		}
		
		inline Type &operator[](int index) { return data[index]; }
		inline const Type &operator[](int index) const { return data[index]; }
		
		inline int size() const { return length; }
		
		void resize(int size) {
			if(size <= capacity) {
				length = size;
				return;
			}
			capacity = size;
			Type *new_data = new Type[capacity];
			for(int i = 0; i < length; i++) {
				new_data[i] = data[i];
			}
			delete [] data;
			data = new_data;
			length = size;
		}
		
		void reserve(int size) {
			if(size <= capacity) {
				return;
			}
			capacity = size;
			Type *new_data = new Type[capacity];
			for(int i = 0; i < length; i++) {
				new_data[i] = data[i];
			}
			delete [] data;
			data = new_data;
		}
		
		void clear() {
			length = 0;
		}
		
		int find(const Type &t) {
			for(int i = 0; i < length; i++) {
				if(data[i] == t) return i;
			}
			return -1;
		}
		
		void append(const Type &t) {
			if(length + 1 <= capacity) {
				data[length++] = t;
				return;
			}
			int old_length = length;
			resize(length * 2 + 1);
			length = old_length + 1;
			data[old_length] = t;
		}
		void append(int position,const Type &t) {
			if(length + 1 <= capacity) {
				for(int i = length - 1; i >= position; i--) {
					data[i + 1] = data[i];
				}
				data[position] = t;
				length++;
				return;
			}
			capacity = length * 2 + 1;
			Type *new_data = new Type[capacity];
			for(int i = 0; i < position; i++) {
				new_data[i] = data[i];
			}
			new_data[position] = t;
			for(int i = position; i < length; i++) {
				new_data[i + 1] = data[i];
			}
			delete [] data;
			data = new_data;
			length++;
		}
		
		void remove() {
			length--;
		}
		void remove(int position) {
			for(int i = position; i < length - 1; i++) {
				data[i] = data[i + 1];
			}
			length--;
		}
		
	private:
		
		int length;
		int capacity;
		Type *data;
};

#endif /* __VECTOR_H__ */
