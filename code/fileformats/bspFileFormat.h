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
// bspFileFormat.h - structures used in binary .bsp files

#ifndef __BSP_QUAKE3__
#define __BSP_QUAKE3__

#include <shared/typedefs.h>
#include <math/plane.h>
#include <math/aabb.h>

#include "bspFileFormat_q2.h"

// original BSP structures designed by ID Software
// used in their Quake3 game
#define BSP_IDENT_IBSP	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
// MoHAA bsp ident
#define BSP_IDENT_2015	(('5'<<24)+('1'<<16)+('0'<<8)+'2')
// MoHBT/MoHSH bsp ident
#define BSP_IDENT_EALA	(('A'<<24)+('L'<<16)+('A'<<8)+'E')

#define BSP_VERSION_Q3		46
#define BSP_VERSION_RTCW	47
#define BSP_VERSION_ET		47
#define BSP_VERSION_COD1	59

#define BSP_VERSION_MOHAA			19
#define BSP_VERSION_MOHSH			20
#define BSP_VERSION_MOHBT			21

// Quake3 / RTCW / ET lumps
#define	Q3_ENTITIES		0
#define	Q3_SHADERS		1
#define	Q3_PLANES			2
#define	Q3_NODES			3
#define	Q3_LEAVES			4
#define	Q3_LEAFSURFACES	5
#define	Q3_LEAFBRUSHES	6
#define	Q3_MODELS			7
#define	Q3_BRUSHES		8
#define	Q3_BRUSHSIDES		9
#define	Q3_DRAWVERTS		10
#define	Q3_DRAWINDEXES	11
#define	Q3_FOGS			12
#define	Q3_SURFACES		13
#define	Q3_LIGHTMAPS		14
#define	Q3_LIGHTGRID		15
#define	Q3_VISIBILITY		16
#define	Q3_LUMPS		17

// MoHAA/MoHSH/MoHBT/MoHPA lumps
#define MOH_SHADERS			0
#define MOH_PLANES				1
#define MOH_LIGHTMAPS			2
#define MOH_SURFACES			3
#define MOH_DRAWVERTS			4
#define MOH_DRAWINDEXES		5
#define MOH_LEAFBRUSHES		6
#define MOH_LEAFSURFACES		7
#define MOH_LEAVES				8
#define MOH_NODES				9
#define MOH_SIDEEQUATIONS				10
#define MOH_BRUSHSIDES			11
#define MOH_BRUSHES			12
#define MOH_MODELS				13
#define MOH_ENTITIES			14
#define MOH_VISIBILITY			15
#define MOH_LIGHTGRIDPALETTE	16
#define MOH_LIGHTGRIDOFFSETS	17
#define MOH_LIGHTGRIDDATA		18
#define MOH_SPHERELIGHTS		19
#define MOH_SPHERELIGHTVIS		20
#define MOH_DUMMY5				21
#define MOH_TERRAIN			22
#define MOH_TERRAININDEXES		23
#define MOH_STATICMODELDATA	24
#define MOH_STATICMODELDEF		25
#define MOH_STATICMODELINDEXES	26
#define MOH_DUMMY10			27

#define	MOH_LUMPS		28

// Call of Duty lumps
#define	COD1_SHADERS		0
#define	COD1_LIGHTMAPS		1
#define	COD1_PLANES			2
#define	COD1_BRUSHSIDES		3
#define	COD1_BRUSHES		4
// lump 5 unknown
#define	COD1_SURFACES		6 // lump 6 triangle soups
#define	COD1_DRAWVERTS		7
#define	COD1_DRAWINDEXES	8
#define COD1_CULLGROUPS 9 // Lump[9] - Cull Groups // 32 bytes per entry. 
#define COD1_CULLINDEXES 10 // Lump[10] - Cull Group Indexes  //4 bytes per entry. 
#define COD1_PORTALVERTS 11 // Lump[11] - Portal Verts  12 bytes per entry.
#define COD1_OCCLUDERS 12 // Lump[12] - Occludersc - 20 bytes per entry. 
#define COD1_OCCLUDERPLANES 13 // Lump[13] - Occluder Planes 4 bytes per entry. 
#define COD1_OCCLUDEREDGES 14 // Lump[14] - Occluder Edges 4 bytes per entry. 
#define COD1_OCCLUDERINDEXES 15 // Lump[15] - Occluder Indexes 2 bytes per entry. 
#define COD1_AABBTREES 16 // Lump[16] - AABB Trees 12 bytes per entry.
#define	COD1_CELLS 17 // Lump[17] - Cells 52 bytes per entry.
#define	COD1_PORTALS 18 // Lump[18] - Portals 16 bytes per entry. 
/* Lump[19] - Light Indexes 
2 bytes per entry.*/
#define	COD1_NODES			20
#define	COD1_LEAFS			21
#define	COD1_LEAFBRUSHES	22
#define	COD1_LEAFSURFACES	23
// Lump[24] - Patch Collision - 16 bytes per entry
// Lump[25] - Collision Verts - 12 bytes per entry. 
// Lump[26] - Collision Indexes - 2 bytes per entry.
#define	COD1_MODELS			27
#define	COD1_VISIBILITY		28
#define	COD1_ENTITIES		29
// Lump[30] - Lights 72 bytes per entry.
//#define	COD1_FOGS			12
//#define	COD1_SURFACES		13
//#define	COD1_LIGHTGRID		15
#define	COD1_LUMPS		32

struct q3Model_s {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
};

struct q3BSPMaterial_s {
	char		shader[64];
	int			surfaceFlags;
	int			contentFlags;
};

// planes x^1 is allways the opposite of plane x
struct q3Plane_s {
	float normal[3];
	float dist;
	
	float distance(const vec3_c &point) const {
		float d = (normal[0]*point.x+normal[1]*point.y+normal[2]*point.z) - dist;
		return d;
	}
	planeSide_e onSide(const vec3_c &p) const {
		float d = distance(p);
		if(d < 0) {
			return SIDE_BACK;
		}
		return SIDE_FRONT;
	}
	planeSide_e onSide(const aabb &bb) const {
#if 0
		planeSide_e s = onSide(bb.getPoint(0));
		for(u32 i = 1; i < 8; i++) {
			planeSide_e s2 = onSide(bb.getPoint(i));
			if(s2 != s) {
				return SIDE_CROSS;
			}
		}
		return s;
#elif 0

	// unoptimized, general code
	vec3_t	corners[2];
	for (int i = 0; i < 3; i++) {
		if (this->normal[i] < 0) {
			corners[0][i] = bb.mins[i];
			corners[1][i] = bb.maxs[i];
		} else {
			corners[1][i] = bb.mins[i];
			corners[0][i] = bb.maxs[i];
		}
	}
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
	float dist1 = DotProduct (this->normal, corners[0]) - this->dist;
	float dist2 = DotProduct (this->normal, corners[1]) - this->dist;
	bool front = false;
	if (dist1 >= 0) {
		if (dist2 < 0)
			return SIDE_CROSS;
		return SIDE_FRONT;
	}
	if (dist2 < 0)
		return SIDE_BACK;
#else
		vec3_t corners[2];
		for (int i = 0; i < 3; i++) {
			if (this->normal[i] > 0) {
				corners[0][i] = bb.mins[i];
				corners[1][i] = bb.maxs[i];
			} else {
				corners[1][i] = bb.mins[i];
				corners[0][i] = bb.maxs[i];
			}
		}
		float dist1 = this->distance(corners[0]);
		float dist2 = this->distance(corners[1]);
		bool front = false;
		if (dist1 >= 0) {
			if (dist2 < 0)
				return SIDE_CROSS;
			return SIDE_FRONT;
		}
		if (dist2 < 0)
			return SIDE_BACK;
		//assert(0); // this could happen only if AABB mins are higher than maxs
		return SIDE_CROSS;
#endif
	}

};

struct q3Node_s {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
};

struct q3Leaf_s {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
};

struct cod1Leaf_s {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;
/*
	// they are not present here in Call Of Duty 1
	int			mins[3];			// for frustum culling
	int			maxs[3];
*/
	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;

	int			cellNum; // may be -1
	int			dummy1;
	int			dummy2;
};


struct q3BrushSide_s {
	int			planeNum;			// positive plane side faces out of the leaf
	int			materialNum;
};

struct q3Brush_s {
	int			firstSide;
	int			numSides;
	int			materialNum;		// the shader that determines the contents flags
};

// CoD1 brush structure is totally different from q3
struct cod1Brush_s {
	short		numSides;
	short		materialNum;		// the shader that determines the contents flags
}; // sizeof(cod1Brush)==4

struct q3Fog_s {
	char		shader[64];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
};

struct q3Vert_s {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
};

enum q3mapSurfaceType_e {
	Q3MST_BAD,
	Q3MST_PLANAR,
	Q3MST_PATCH,
	Q3MST_TRIANGLE_SOUP,
	Q3MST_FLARE
};

struct q3Surface_s {
	int			materialNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts;

	int			firstIndex;
	int			numIndexes;

	int			lightmapNum;
	int			lightmapX, lightmapY;
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int			patchWidth;
	int			patchHeight;
};

// the very simplified Call Of Duty surface struct.
struct cod1Surface_s {
	u16			materialNum;
	s16			lightmapNum; // that certainly not a surfaceType (has value 12 on stalingrad.bsp); it must be signed because -1 index means that surf has no lightmap

	u32			firstVert;
	u16			numVerts;

	u16			numIndexes;
	u32			firstIndex;
};

struct visHeader_s {
	int numClusters;
	int clusterSize; // in bytes
	byte data[4]; // variable-sized
};

struct lump_s {
	u32 fileOfs;
	u32 fileLen;
};

struct q3Header_s {
	int			ident;
	int			version;
	lump_s		lumps[Q3_LUMPS];

	const lump_s *getLumps() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			// MoH maps have checksum integer before lumps
			return ((const lump_s*)(((const byte*)&lumps[0])+4));
		} else {
			return &lumps[0];
		}
	}
	const q3Model_s *getModels() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Model_s*)(((const byte*)this)+getLumps()[MOH_MODELS].fileOfs);
		} else {
			return (const q3Model_s*)(((const byte*)this)+getLumps()[Q3_MODELS].fileOfs);
		}
	}
	const q3Model_s *getModel(u32 modelNum) const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Model_s*)(((const byte*)this)+getLumps()[MOH_MODELS].fileOfs+modelNum*sizeof(q3Model_s));
		} else if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			return (const q3Model_s*)(((const byte*)this)+getLumps()[COD1_MODELS].fileOfs+modelNum*(sizeof(q3Model_s)+8));
		} else {
			return (const q3Model_s*)(((const byte*)this)+getLumps()[Q3_MODELS].fileOfs+modelNum*sizeof(q3Model_s));
		}
	}
	u32 getNumModels() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return getLumps()[MOH_MODELS].fileLen/sizeof(q3Model_s);
		} else if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			return getLumps()[COD1_MODELS].fileLen/(sizeof(q3Model_s)+8);
		} else {
			return getLumps()[Q3_MODELS].fileLen/sizeof(q3Model_s);
		}
	}
	const q3BSPMaterial_s *getMaterials() const {
		return (const q3BSPMaterial_s*)(((const byte*)this)+getLumps()[Q3_SHADERS].fileOfs);
	}
	const q3Plane_s *getPlanes() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Plane_s*)(((const byte*)this)+getLumps()[MOH_PLANES].fileOfs);
		} else {
			return (const q3Plane_s*)(((const byte*)this)+getLumps()[Q3_PLANES].fileOfs);
		}
	}
	const q3Node_s *getNodes() const {
		return (const q3Node_s*)(((const byte*)this)+lumps[Q3_NODES].fileOfs);
	}
	const q3Leaf_s *getLeaves() const {
		return (const q3Leaf_s*)(((const byte*)this)+lumps[Q3_LEAVES].fileOfs);
	}
	const q3BrushSide_s *getBrushSides() const {
		return (const q3BrushSide_s*)(((const byte*)this)+lumps[Q3_BRUSHSIDES].fileOfs);
	}
	const q3Brush_s *getBrushes() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Brush_s*)(((const byte*)this)+getLumps()[MOH_BRUSHES].fileOfs);
		} else {
			return (const q3Brush_s*)(((const byte*)this)+getLumps()[Q3_BRUSHES].fileOfs);
		}
	}
	u32 getNumBrushes() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return getLumps()[MOH_BRUSHES].fileLen / sizeof(q3Brush_s);
		} else {
			return getLumps()[Q3_BRUSHES].fileLen / sizeof(q3Brush_s);
		}
	}
	const q3Vert_s *getVerts() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Vert_s*)(((const byte*)this)+getLumps()[MOH_DRAWVERTS].fileOfs);
		} else if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			return (const q3Vert_s*)(((const byte*)this)+getLumps()[COD1_DRAWVERTS].fileOfs);
		} else {
			return (const q3Vert_s*)(((const byte*)this)+getLumps()[Q3_DRAWVERTS].fileOfs);
		}
	}
	const q3Surface_s *getSurfaces() const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Surface_s*)(((const byte*)this)+getLumps()[MOH_SURFACES].fileOfs);
		} else {
			return (const q3Surface_s*)(((const byte*)this)+getLumps()[Q3_SURFACES].fileOfs);
		}
	}
	const cod1Surface_s *getCoD1Surfaces() const {
		return (const cod1Surface_s*)(((const byte*)this)+getLumps()[COD1_SURFACES].fileOfs);
	}
	const q3Surface_s *getNextSurface(const q3Surface_s *sf) const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Surface_s*)(((const byte*)sf)+(sizeof(q3Surface_s)+4));
		} else {
			return (const q3Surface_s*)(((const byte*)sf)+sizeof(q3Surface_s));
		}
	}
	u32 getModelStructSize() const {
		if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			// CoD1 model struct is 8 bytes larger
			return (sizeof(q3Model_s)+8);
		}
		// Q3, RTCW, ET, and MoH are using the classic q3Model_s struct
		return (sizeof(q3Model_s));	
	}
	const q3Model_s *getNextModel(const q3Model_s *mod) const {
		if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			// CoD1 model struct is 8 bytes larger
			return (const q3Model_s*)(((const byte*)mod)+(sizeof(q3Model_s)+8));
		}
		// Q3, RTCW, ET, and MoH are using the classic q3Model_s struct
		return (const q3Model_s*)(((const byte*)mod)+(sizeof(q3Model_s)));
	}
	const q3Surface_s *getSurface(u32 surfNum) const {
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			return (const q3Surface_s*)(((const byte*)this)+getLumps()[MOH_SURFACES].fileOfs+(sizeof(q3Surface_s)+4)*surfNum);
		} else if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			return 0; // dont use this function for CoD bsps
		} else {
			return (const q3Surface_s*)(((const byte*)this)+getLumps()[Q3_SURFACES].fileOfs+sizeof(q3Surface_s)*surfNum);
		}
	}
	const q3BSPMaterial_s *getMat(u32 matNum) const {
		u32 matSize;
		const byte *p;
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			// MoHAA has extended material struct
			matSize = sizeof(q3BSPMaterial_s) + 64 + 4;
			p = getLumpData(MOH_SHADERS);
		} else if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1) {
			matSize = sizeof(q3BSPMaterial_s);
			p = getLumpData(COD1_SHADERS);
		} else {
			matSize = sizeof(q3BSPMaterial_s);
			p = getLumpData(Q3_SHADERS);
		}
		return (const q3BSPMaterial_s*)(p + matSize * matNum);
	}
	const q3BrushSide_s *getBrushSide(u32 bsNum) const {
		u32 sideSize;
		const byte *p;
		if(ident == BSP_IDENT_2015 || ident == BSP_IDENT_EALA) {
			// MoHAA has extended brushSide struct (equationNum integer)
			sideSize = sizeof(q3BrushSide_s) + 4;
			p = getLumpData(MOH_BRUSHSIDES);
		} else {
			sideSize = sizeof(q3BrushSide_s);
			p = getLumpData(Q3_BRUSHSIDES);
		}
		return (const q3BrushSide_s*)(p + sideSize * bsNum);
	}
	const byte *getLumpData(u32 lumpNum) const {
		return (const byte*)(((const byte*)this)+getLumps()[lumpNum].fileOfs);
	}
	const u32 getLumpStructNum(u32 lumpNum, u32 structSize) const {
		return (getLumps()[lumpNum].fileLen / structSize);
	}

	bool isBSPCoD1() const {
		if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_COD1)
			return true;
		return false;
	}
	bool isBSPQ2() const {
		if(ident == BSP_IDENT_IBSP && version == BSP_VERSION_Q2)
			return true;
		return false;
	}
	
	// for some reasons Call Of Duty bsps has swapped lump_t fields.
	// We need to swap them back before loading the map
	void swapCoDLumpLenOfsValues() {
		if(ident != BSP_IDENT_IBSP || version != BSP_VERSION_COD1) 
			return; // that's not a COD bsp
		for(u32 i = 0; i < COD1_LUMPS; i++) {
			u32 tmp = lumps[i].fileLen;
			lumps[i].fileLen = lumps[i].fileOfs;
			lumps[i].fileOfs = tmp;
		}
	}
	u32 getLumpSize(u32 lumpNum) const {
		return getLumps()[lumpNum].fileLen;
	}
	u32 getLumpStructCount(u32 lumpNum, u32 elemSize) const {
		return getLumps()[lumpNum].fileLen / elemSize;
	}
};

#endif // __BSP_QUAKE3__
