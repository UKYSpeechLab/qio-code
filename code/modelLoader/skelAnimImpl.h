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
// skelAnimImpl.h - implementation of skelAnimAPI_i
#ifndef __SKELANIMIMPL_H__
#define __SKELANIMIMPL_H__

#include "sk_local.h"
#include <api/skelAnimAPI.h>
#include <math/aabb.h>
#include <shared/array.h>
#include <shared/str.h>

class skelFrameMD5_c {

};

struct md5BoneVal_s {
	float pos[4];
	float quat[4]; // W component will be calculated
};
struct md5AnimBone_s {
	short firstComponent;
	short componentFlags;
};

enum {
	COMPONENT_BIT_TX = 1 << 0,
	COMPONENT_BIT_TY = 1 << 1,
	COMPONENT_BIT_TZ = 1 << 2,
	COMPONENT_BIT_QX = 1 << 3,
	COMPONENT_BIT_QY = 1 << 4,
	COMPONENT_BIT_QZ = 1 << 5
};

class md5Frame_c {
friend class skelAnimMD5_c;
	arraySTD_c<float> components;
	aabb bounds;
};
class skelAnimMD5_c : public skelAnimAPI_i {
	str animFileName;
	float frameRate;
	arraySTD_c<md5Frame_c> frames;
	boneDefArray_c bones;
	arraySTD_c<md5AnimBone_s> md5AnimBones;
	arraySTD_c<md5BoneVal_s> baseFrame;

	virtual const char *getName() const {
		return animFileName;
	}
	virtual u32 getNumFrames() const {
		return frames.size();
	}
	virtual u32 getNumBones() const {
		return bones.size();
	}
	// boneDefs array might be not present for some other animation types than md5 (??)
	virtual const class boneDefArray_c *getBoneDefs() const {
		return &bones;
	}

	virtual void buildFrameBonesLocal(u32 frameNum, class boneOrArray_c &out) const;
public:
	bool loadMD5Anim(const char *fname);
};

#endif //__SKELANIMIMPL_H__