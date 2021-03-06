/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <string.h>
#include <math.h>
#include "q3mapcommon/cmdlib.h"
#include "q3mapcommon/mathlib.h"
#include "q3mapcommon/imagelib.h"
#include "q3mapcommon/scriplib.h"
#include "q3mapcommon/qfiles.h"
#include "q3mapcommon/surfaceflags.h"

#include "shaders.h"
#include "../q3radiant/libs/pakstuff.h"

// 5% backsplash by default
#define	DEFAULT_BACKSPLASH_FRACTION		0.05
#define	DEFAULT_BACKSPLASH_DISTANCE		24


#define	MAX_SURFACE_INFO	4096

shaderInfo_t	defaultInfo;
shaderInfo_t	shaderInfo[MAX_SURFACE_INFO];
int				numShaderInfo;


typedef struct {
	char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// server relevant contents
	{"water",		1,	0,	CONTENTS_WATER },
	{"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
	{"lava",		1,	0,	CONTENTS_LAVA },		// very damaging
	{"playerclip",	1,	0,	CONTENTS_PLAYERCLIP },
	{"monsterclip",	1,	0,	CONTENTS_MONSTERCLIP },
	{"nodrop",		1,	0,	CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid",	1,	SURF_NONSOLID,	0},						// clears the solid flag

	// utility relevant attributes
	{"origin",		1,	0,	CONTENTS_ORIGIN },		// center of rotating brushes
	{"trans",		0,	0,	CONTENTS_TRANSLUCENT },	// don't eat contained surfaces
	{"detail",		0,	0,	CONTENTS_DETAIL },		// don't include in structural bsp
	{"structural",	0,	0,	CONTENTS_STRUCTURAL },	// force into structural bsp even if trnas
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"clusterportal",1, 0,  CONTENTS_CLUSTERPORTAL },// for bots
	{"donotenter",  1,  0,  CONTENTS_DONOTENTER },	// for bots
#ifdef CONTENTS_BOTCLIP
	{"botclip",     1,  0,  CONTENTS_BOTCLIP },		// for bots
#endif // CONTENTS_BOTCLIP
#ifdef CONTENTS_NOBOTCLIP
	{"nobotclip",	0,	0,	CONTENTS_NOBOTCLIP },	// don't use for bot clipping
#endif

	{"fog",			1,	0,	CONTENTS_FOG},			// carves surfaces entering
	{"sky",			0,	SURF_SKY,		0 },		// emit light from an environment map
	{"lightfilter",	0,	SURF_LIGHTFILTER, 0 },		// filter light going through it
	{"alphashadow",	0,	SURF_ALPHASHADOW, 0 },		// test light on a per-pixel basis
	{"hint",		0,	SURF_HINT,		0 },		// use as a primary splitter

	// server attributes
	{"slick",		0,	SURF_SLICK,		0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nomarks",		0,	SURF_NOMARKS,	0 },		// don't make impact marks, but still explode
	{"ladder",		0,	SURF_LADDER,	0 },
	{"nodamage",	0,	SURF_NODAMAGE,	0 },
	{"metalsteps",	0,	SURF_METALSTEPS,0 },
	{"flesh",		0,	SURF_FLESH,		0 },
	{"nosteps",		0,	SURF_NOSTEPS,	0 },

	// drawsurf attributes
	{"nodraw",		0,	SURF_NODRAW,	0 },		// don't generate a drawsurface (or a lightmap)
	{"pointlight",	0,	SURF_POINTLIGHT, 0 },		// sample lighting at vertexes
	{"nolightmap",	0,	SURF_NOLIGHTMAP,0 },		// don't generate a lightmap
	{"nodlight",	0,	SURF_NODLIGHT, 0 },			// don't ever add dynamic lights
#ifdef SURF_DUST
	{"dust",		0,	SURF_DUST, 0}				// leave dust trail when walking on this surface
#endif
};




// DevIL library for png / dds
#define USE_DEVIL_LIBRARY
#ifdef USE_DEVIL_LIBRARY

#include <IL/il.h>

int b_devilReady = 0;
void IMG_InitDevil() {
	if(b_devilReady)
		return;
	//initialize devIL
	ilInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
	ilEnable(IL_TYPE_SET);
	ilTypeFunc(IL_UNSIGNED_BYTE);

	b_devilReady = 1;
}

void LoadBuff_IL(unsigned char *fbuffer, int len, unsigned char **pic, int *width, int *height, int type) 
{
	ILuint ilTexture;
	ILboolean done = 0;
	int d;

	// ensure that image loading library is ready
	IMG_InitDevil();

	// load texture
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);

	done = ilLoadL(type,fbuffer,len);
	if(!done) {
		ilBindImage(0);
		ilDeleteImages(1, &ilTexture);
		*pic = 0;
		*width = 0;
		*height = 0;
		return;
	}
	*width = ilGetInteger(IL_IMAGE_WIDTH);
	*height = ilGetInteger(IL_IMAGE_HEIGHT);
	d = ilGetInteger(IL_IMAGE_BPP);
	if(d!=4) {
		d = 4;
		ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	}
	*pic = (unsigned char*)malloc((*width) * (*height) * (d));
	memcpy(*pic, ilGetData(), (*width) * (*height) * (d));
    ilBindImage(0);
    ilDeleteImages(1, &ilTexture);
}
void LoadImagePNG(const char *filename, unsigned char **pic, int *width, int *height) 
{
	byte	*fbuffer = NULL;
	int nLen = TryLoadFile( ( char * ) filename, (void **)&fbuffer);
	if (nLen == -1) 
	{
		nLen = PakLoadAnyFile((char*)filename, (void**)&fbuffer);
		if (nLen == -1)
		{
			return;
		}
	}
	LoadBuff_IL(fbuffer, nLen, pic, width, height,IL_PNG);
	free(fbuffer);
}

void LoadImageDDS(const char *filename, unsigned char **pic, int *width, int *height) 
{
	byte	*fbuffer = NULL;
	int nLen = TryLoadFile( ( char * ) filename, (void **)&fbuffer);
	if (nLen == -1) 
	{
		nLen = PakLoadAnyFile((char*)filename, (void**)&fbuffer);
		if (nLen == -1)
		{
			return;
		}
	}
	LoadBuff_IL(fbuffer, nLen, pic, width, height,IL_DDS);
	free(fbuffer);
}
void LoadImageJPG(const char *filename, unsigned char **pic, int *width, int *height) 
{
	byte	*fbuffer = NULL;
	int nLen = TryLoadFile( ( char * ) filename, (void **)&fbuffer);
	if (nLen == -1) 
	{
		nLen = PakLoadAnyFile((char*)filename, (void**)&fbuffer);
		if (nLen == -1)
		{
			return;
		}
	}
	LoadBuff_IL(fbuffer, nLen, pic, width, height,IL_JPG);
	free(fbuffer);
}
#endif
//==============

void LoadImageData(const char *name, byte **pic, int *w, int *h)
{
	char fixed[512];
	int len;
	int i;

	*pic = 0;

	strcpy(fixed,name);
	len = strlen(fixed);
	strcpy(fixed+len-3,"png");
	LoadImagePNG(fixed,pic,w,h);
	if(*pic)
		return;
	strcpy(fixed+len-3,"jpg");
	LoadImageJPG(fixed,pic,w,h);
	if(*pic)
		return;
	strcpy(fixed+len-3,"dds");
	LoadImageDDS(fixed,pic,w,h);
	if(*pic)
		return;
	strcpy(fixed+len-3,"tga");
	LoadTGA(fixed,pic,w,h);
	if(*pic)
		return;

}



/*
===============
LoadShaderImage
===============
*/
static void LoadShaderImage( shaderInfo_t *si ) {
	char			filename[1024];
	int count;
	float color[4];
	int i;
	
	si->pixels = 0;
	// look for	 the editorimage if it is specified
	if ( si->editorimage[0] ) {
		sprintf( filename, "%s/%s", gamedir, si->editorimage );
		DefaultExtension( filename, ".tga" );
		LoadImageData(filename,&si->pixels,&si->width,&si->height);

	}
	if ( si->pixels == NULL) {
		// just try the shader name with a .tga
		// on unix, we have case sensitivity problems...
		strcpy(filename, gamedir);
		strcat(filename,"/");
		strcat(filename,si->shader);
		strcat(filename,".tga");
		/// sprintf( filename, "%s%s.tga", gamedir, si->shader );
		LoadImageData(filename,&si->pixels,&si->width,&si->height);
	}


	if ( si->pixels == NULL) {
		// couldn't load anything
	  _printf("WARNING: Couldn't find image for shader %s (editorImage name %s)\n", si->shader, si->editorimage );

		si->color[0] = 1;
		si->color[1] = 1;
		si->color[2] = 1;
		si->width = 64;
		si->height = 64;
		si->pixels = malloc( si->width * si->height * 4 );
		memset ( si->pixels, 255, si->width * si->height * 4 );
		return;
	}

	count = si->width * si->height;

	VectorClear( color );
	color[ 3 ] = 0;
	for ( i = 0 ; i < count ; i++ ) {
		color[0] += si->pixels[ i * 4 + 0 ];
		color[1] += si->pixels[ i * 4 + 1 ];
		color[2] += si->pixels[ i * 4 + 2 ];
		color[3] += si->pixels[ i * 4 + 3 ];
	}
	ColorNormalize( color, si->color );
	VectorScale( color, 1.0/count, si->averageColor );
}

/*
===============
AllocShaderInfo
===============
*/
static shaderInfo_t	*AllocShaderInfo( void ) {
	shaderInfo_t	*si;

	if ( numShaderInfo == MAX_SURFACE_INFO ) {
		Error( "MAX_SURFACE_INFO" );
	}
	si = &shaderInfo[ numShaderInfo ];
	numShaderInfo++;

	// set defaults

	si->contents = CONTENTS_SOLID;

	si->backsplashFraction = DEFAULT_BACKSPLASH_FRACTION;
	si->backsplashDistance = DEFAULT_BACKSPLASH_DISTANCE;

	si->lightmapSampleSize = 0;
	si->forceTraceLight = qfalse;
	si->forceVLight = qfalse;
	si->patchShadows = qfalse;
	si->vertexShadows = qfalse;
	si->noVertexShadows = qfalse;
	si->forceSunLight = qfalse;
	si->vertexScale = 1.0;
	si->notjunc = qfalse;

	return si;
}

/*
===============
ShaderInfoForShader
===============
*/
shaderInfo_t	*ShaderInfoForShader( const char *shaderName ) {
	int				i;
	shaderInfo_t	*si;
	char			shader[MAX_QPATH];

	// strip off extension
	strcpy( shader, shaderName );
	StripExtension( shader );

	// search for it
	for ( i = 0 ; i < numShaderInfo ; i++ ) {
		si = &shaderInfo[ i ];
		if ( !Q_stricmp( shader, si->shader ) ) {
			if ( !si->width ) {
				LoadShaderImage( si );
			}
			return si;
		}
	}

	si = AllocShaderInfo();
	strcpy( si->shader, shader );

	// build in materials
	// (in case that .shader/.mtr files are missing)
	if(!stricmp(shader,"textures/common/areaportal")) {
		printf("Returning build-in areaportal material...\n");
		si->contents = CONTENTS_AREAPORTAL;
	} else if(!stricmp(shader,"textures/common/caulk")) {
		printf("Returning build-in caulk material...\n");
		si->surfaceFlags |= SURF_NODRAW;
	} else if(!stricmp(shader,"textures/common/origin")) {
		printf("Returning build-in origin material...\n");
		si->contents |= CONTENTS_ORIGIN;
	}

	LoadShaderImage( si );

	return si;
}

/*
===============
ParseShaderFile
===============
*/
static void ParseShaderFile( const char *filename ) {
	int		i;
	int		numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
	shaderInfo_t	*si;

//	qprintf( "shaderFile: %s\n", filename );
	LoadScriptFile( filename );
	while ( 1 ) {
		if ( !GetToken( qtrue ) ) {
			break;
		}
		if(!stricmp(token,"table")) {
			int level;
			int len;
			// get table name
			if ( !GetToken( qtrue ) ) {
				break;
			}	
			level = 0;
			len = strlen(token);
			for(i = 0; i < len; i++) {
				if(token[i] == '{') {
					level++;
					token[i] = 0;
				}
			}
			_printf("Parsing table %s\n",token);
			if(level == 0) {
				MatchToken( "{" );
				level = 1;
			}
			while(1) 
			{
				if ( !GetToken( qtrue ) ) {
					break;
				}
				if ( !strcmp( token, "}" ) ) {
					level--;
				}
				if ( !strcmp( token, "{" ) ) {
					level++;
				}
				if(level == 0)
					break;
			}
			continue;
		}

		si = AllocShaderInfo();
		strcpy( si->shader, token );
		MatchToken( "{" );
		while ( 1 ) {
			if ( !GetToken( qtrue ) ) {
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}

			// skip internal braced sections
			if ( !strcmp( token, "{" ) ) {
				si->hasPasses = qtrue;
				while ( 1 ) {
					if ( !GetToken( qtrue ) ) {
						break;
					}
					if ( !strcmp( token, "}" ) ) {
						break;
					}
				}
				continue;
			}

			if ( !Q_stricmp( token, "surfaceparm" ) ) {
				GetToken( qfalse );
				for ( i = 0 ; i < numInfoParms ; i++ ) {
					if ( !Q_stricmp( token, infoParms[i].name ) ) {
						si->surfaceFlags |= infoParms[i].surfaceFlags;
						si->contents |= infoParms[i].contents;
						if ( infoParms[i].clearSolid ) {
							si->contents &= ~CONTENTS_SOLID;
						}
						break;
					}
				}
				if ( i == numInfoParms ) {
					// we will silently ignore all tokens beginning with qer,
					// which are QuakeEdRadient parameters
					if ( Q_strncasecmp( token, "qer", 3 ) ) {
						_printf( "Unknown surfaceparm: \"%s\"\n", token );
					}
				}
				continue;
			}


			// qer_editorimage <image>
			if ( !Q_stricmp( token, "qer_editorimage" ) ) {
				GetToken( qfalse );
				strcpy( si->editorimage, token );
				DefaultExtension( si->editorimage, ".tga" );
				continue;
			}

			// q3map_lightimage <image>
			if ( !Q_stricmp( token, "q3map_lightimage" ) ) {
				GetToken( qfalse );
				strcpy( si->lightimage, token );
				DefaultExtension( si->lightimage, ".tga" );
				continue;
			}

			// q3map_surfacelight <value>
			if ( !Q_stricmp( token, "q3map_surfacelight" )  ) {
				GetToken( qfalse );
				si->value = atoi( token );
				continue;
			}

			// q3map_lightsubdivide <value>
			if ( !Q_stricmp( token, "q3map_lightsubdivide" )  ) {
				GetToken( qfalse );
				si->lightSubdivide = atoi( token );
				continue;
			}

			// q3map_lightmapsamplesize <value>
			if ( !Q_stricmp( token, "q3map_lightmapsamplesize" ) ) {
				GetToken( qfalse );
				si->lightmapSampleSize = atoi( token );
				continue;
			}

			// q3map_tracelight
			if ( !Q_stricmp( token, "q3map_tracelight" ) ) {
				si->forceTraceLight = qtrue;
				continue;
			}

			// q3map_vlight
			if ( !Q_stricmp( token, "q3map_vlight" ) ) {
				si->forceVLight = qtrue;
				continue;
			}

			// q3map_patchshadows
			if ( !Q_stricmp( token, "q3map_patchshadows" ) ) {
				si->patchShadows = qtrue;
				continue;
			}

			// q3map_vertexshadows
			if ( !Q_stricmp( token, "q3map_vertexshadows" ) ) {
				si->vertexShadows = qtrue;
				continue;
			}

			// q3map_novertexshadows
			if ( !Q_stricmp( token, "q3map_novertexshadows" ) ) {
				si->noVertexShadows = qtrue;
				continue;
			}

			// q3map_forcesunlight
			if ( !Q_stricmp( token, "q3map_forcesunlight" ) ) {
				si->forceSunLight = qtrue;
				continue;
			}

			// q3map_vertexscale
			if ( !Q_stricmp( token, "q3map_vertexscale" ) ) {
				GetToken( qfalse );
				si->vertexScale = atof(token);
				continue;
			}

			// q3map_notjunc
			if ( !Q_stricmp( token, "q3map_notjunc" ) ) {
				si->notjunc = qtrue;
				continue;
			}

			// q3map_globaltexture
			if ( !Q_stricmp( token, "q3map_globaltexture" )  ) {
				si->globalTexture = qtrue;
				continue;
			}

			// q3map_backsplash <percent> <distance>
			if ( !Q_stricmp( token, "q3map_backsplash" ) ) {
				GetToken( qfalse );
				si->backsplashFraction = atof( token ) * 0.01;
				GetToken( qfalse );
				si->backsplashDistance = atof( token );
				continue;
			}

			// q3map_backshader <shader>
			if ( !Q_stricmp( token, "q3map_backshader" ) ) {
				GetToken( qfalse );
				strcpy( si->backShader, token );
				continue;
			}

			// q3map_flare <shader>
			if ( !Q_stricmp( token, "q3map_flare" ) ) {
				GetToken( qfalse );
				strcpy( si->flareShader, token );
				continue;
			}

			// light <value> 
			// old style flare specification
			if ( !Q_stricmp( token, "light" ) ) {
				GetToken( qfalse );
				strcpy( si->flareShader, "flareshader" );
				continue;
			}

			// q3map_sun <red> <green> <blue> <intensity> <degrees> <elivation>
			// color will be normalized, so it doesn't matter what range you use
			// intensity falls off with angle but not distance 100 is a fairly bright sun
			// degree of 0 = from the east, 90 = north, etc.  altitude of 0 = sunrise/set, 90 = noon
			if ( !Q_stricmp( token, "q3map_sun" ) ) {
				float	a, b;

				GetToken( qfalse );
				si->sunLight[0] = atof( token );
				GetToken( qfalse );
				si->sunLight[1] = atof( token );
				GetToken( qfalse );
				si->sunLight[2] = atof( token );
				
				VectorNormalize( si->sunLight, si->sunLight);

				GetToken( qfalse );
				a = atof( token );
				VectorScale( si->sunLight, a, si->sunLight);

				GetToken( qfalse );
				a = atof( token );
				a = a / 180 * Q_PI;

				GetToken( qfalse );
				b = atof( token );
				b = b / 180 * Q_PI;

				si->sunDirection[0] = cos( a ) * cos( b );
				si->sunDirection[1] = sin( a ) * cos( b );
				si->sunDirection[2] = sin( b );

				si->surfaceFlags |= SURF_SKY;
				continue;
			}

			// tesssize is used to force liquid surfaces to subdivide
			if ( !Q_stricmp( token, "tesssize" ) ) {
				GetToken( qfalse );
				si->subdivisions = atof( token );
				continue;
			}

			// cull none will set twoSided
			if ( !Q_stricmp( token, "cull" ) ) {
				GetToken( qfalse );
				if ( !Q_stricmp( token, "none" ) ) {
					si->twoSided = qtrue;
				}
				continue;
			}


			// deformVertexes autosprite[2]
			// we catch this so autosprited surfaces become point
			// lights instead of area lights
			if ( !Q_stricmp( token, "deformVertexes" ) ) {
				GetToken( qfalse );
				if ( !Q_strncasecmp( token, "autosprite", 10 ) ) {
					si->autosprite = qtrue;
          si->contents = CONTENTS_DETAIL;
				}
				continue;
			}


			// ignore all other tokens on the line

			while ( TokenAvailable() ) {
				GetToken( qfalse );
			}
		}			
	}
}

/*
===============
LoadShaderInfo
===============
*/
#include <io.h>
void LoadShaderInfo( void ) {
	char	dirstring[1024];
	struct _finddata_t fileinfo;
	int		handle;

	sprintf (dirstring, "%s/materials/*.mtr", gamedir);

	//qprintf(dirstring);
  handle = _findfirst (dirstring, &fileinfo);
  if (handle != -1)
  {
    do
    {
      if ((fileinfo.attrib & _A_SUBDIR))
        continue;
      sprintf(dirstring, "%s/materials/%s", gamedir, fileinfo.name);
	qprintf("Trying to load %s\n",dirstring);
      ParseShaderFile(dirstring);
	  } while (_findnext( handle, &fileinfo ) != -1);

	  _findclose (handle);
  }

	qprintf( "%5i shaderInfo\n", numShaderInfo);
}

