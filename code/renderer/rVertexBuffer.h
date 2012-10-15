/*
============================================================================
Copyright (C) 2012 V.

This file is part of Qio source code.

Qio source code is free software; you can redistribute it 
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Qio source code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA,
or simply visit <http://www.gnu.org/licenses/>.
============================================================================
*/
// rVertexBuffer.h 
#ifndef __RVERTEXBUFFER_H__
#define __RVERTEXBUFFER_H__

#include <math/vec3.h>
#include <math/vec2.h>
#include <shared/array.h>

class rVert_c {
public:
	vec3_c xyz;
	vec3_c normal;
	vec2_c tc;
	vec2_c lc;
	byte color[4];
	//vec2_c tan;
	//vec2_c bin;

	// quadratic interpolation for n-dimensional vector
	inline void mc_getInterpolated_quadraticn(int rows, float *out, const float *v1, const float *v2, const float *v3, f32 d)
	{
		const f32 inv = 1.0 - d;
		const f32 mul0 = inv * inv;
		const f32 mul1 =  2.0 * d * inv;
		const f32 mul2 = d * d;
		for(int i = 0; i < rows; i++) {
			out[i] = (v1[i] * mul0 + v2[i] * mul1 + v3[i] * mul2);
		}
	}
	// returns the result of quadratic interpolation between this vertex and two other vertices
	rVert_c getInterpolated_quadratic(rVert_c &a, rVert_c &b, float s) {
		rVert_c out;
		mc_getInterpolated_quadraticn(3,out.xyz,xyz,a.xyz,b.xyz,s);
		mc_getInterpolated_quadraticn(2,out.tc,tc,a.tc,b.tc,s);
		mc_getInterpolated_quadraticn(2,out.lc,lc,a.lc,b.lc,s);
		mc_getInterpolated_quadraticn(3,out.normal,normal,a.normal,b.normal,s);
		//mc_getInterpolated_quadraticn(3,out.bin,bin,a.bin,b.bin,s);
		//mc_getInterpolated_quadraticn(3,out.tan,tan,a.tan,b.tan,s);
		vec3_c ct, ca, cb;
		ct.fromByteRGB(this->color);
		ca.fromByteRGB(a.color);
		cb.fromByteRGB(b.color);
		vec3_c res;
		mc_getInterpolated_quadraticn(3,res,ct,ca,cb,s);
		res.colorToBytes(out.color);
		return out;
	}
};
class rVertexBuffer_c {
	arraySTD_c<rVert_c> data;
	union {
		u32 handleU32;
		void *handleV;
	};
public:
	void resize(u32 newSize) {
		data.resize(newSize);
	}
	void push_back(const rVert_c &nv) {
		data.push_back(nv);
	}
	u32 size() const {
		return data.size();
	}
	const rVert_c &operator [] (u32 index) const {
		return data[index];
	}
	rVert_c &operator [] (u32 index) {
		return data[index];
	}
	const rVert_c *getArray() const {
		return data.getArray();
	}
	rVert_c *getArray() {
		return data.getArray();
	}
	void destroy() {
		data.clear();
	}
};

#endif // __RVERTEXBUFFER_H__