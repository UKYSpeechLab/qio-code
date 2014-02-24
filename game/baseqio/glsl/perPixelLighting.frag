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
// glsl/perPixelLighting.frag - per pixel lighting shader for OpenGL backend

// shader input
uniform sampler2D colorMap;
uniform vec3 u_lightOrigin;
uniform float u_lightRadius;
// shader varying variables
varying vec3 v_vertXYZ;
varying vec3 v_vertNormal; 

#if defined(HAS_BUMP_MAP) || defined(HAS_HEIGHT_MAP)
attribute vec3 atrTangents;
attribute vec3 atrBinormals;
varying mat3 tbnMat;
#endif
#ifdef HAS_BUMP_MAP
uniform sampler2D bumpMap;
#endif
#ifdef HAS_HEIGHT_MAP
uniform sampler2D heightMap;
varying vec3 v_tbnEyeDir;
#endif
#if defined(HAS_HEIGHT_MAP) && defined(USE_RELIEF_MAPPING)
#include "reliefMappingRaycast.inc"
#endif

#ifdef SHADOW_MAPPING_POINT_LIGHT
uniform mat4 u_entityMatrix;
varying vec4 shadowCoord0;
varying vec4 shadowCoord1;
varying vec4 shadowCoord2;
varying vec4 shadowCoord3;
varying vec4 shadowCoord4;
varying vec4 shadowCoord5;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;

int cubeSide(vec3 v) {
	vec3 normals[] = { vec3(1,0,0), vec3(-1,0,0),
						vec3(0,1,0), vec3(0,-1,-0),
						vec3(0,0,1), vec3(0,0,-1)};	
	float max = 0;
	int ret;
	for(int i = 0; i < 6; i++) {
		float d = dot(normals[i],v);
		if(d < max) {
			max = d;
			ret = i;
		}
	}
	return ret;
}
#ifdef ENABLE_SHADOW_MAPPING_BLUR
float doShadowBlurSample(sampler2DShadow map, vec4 coord)
{
	float shadow = 0.0;
	
	float pixelOffset = 1.0/2048.0;
	//float samples = 0;
	// avoid counter shadow
	if (coord.w > 1.0)
	{
		float x,y;
		for (y = -1.5; y <=1.5; y+=1.0)
	    {
			for (x = -1.5; x <=1.5; x+=1.0)
		    {
				//if(
				//continue;
				vec4 c = coord + vec4(x * pixelOffset * coord.w, y * pixelOffset * coord.w, 0, 0.0);
				shadow += shadow2DProj(map, c ).w;
				//samples += 1.0;
			}
		}	
		//shadow /= samples;
		shadow /= 16;
	}
	return shadow;
}
#endif
float computeShadow(vec3 lightToVertDirection) {
	float shadow = 0.0;
  	int side = cubeSide(lightToVertDirection);
#ifndef ENABLE_SHADOW_MAPPING_BLUR
	if (side == 0) {
		shadow += shadow2DProj(shadowMap0, shadowCoord0).s;
	} else if(side == 1) {
		shadow += shadow2DProj(shadowMap1, shadowCoord1).s;
	} else if(side == 2) {
		shadow += shadow2DProj(shadowMap2, shadowCoord2).s;
	} else if(side == 3) {
		shadow += shadow2DProj(shadowMap3, shadowCoord3).s;
	} else if(side == 4) {
		shadow += shadow2DProj(shadowMap4, shadowCoord4).s;
	} else if(side == 5) {
		shadow += shadow2DProj(shadowMap5, shadowCoord5).s;
	} else {
		// never gets here
	}
#else
	if (side == 0) {
		shadow += doShadowBlurSample(shadowMap0, shadowCoord0);
	} else if(side == 1) {
		shadow += doShadowBlurSample(shadowMap1, shadowCoord1);
	} else if(side == 2) {
		shadow += doShadowBlurSample(shadowMap2, shadowCoord2);
	} else if(side == 3) {
		shadow += doShadowBlurSample(shadowMap3, shadowCoord3);
	} else if(side == 4) {
		shadow += doShadowBlurSample(shadowMap4, shadowCoord4);
	} else if(side == 5) {
		shadow += doShadowBlurSample(shadowMap5, shadowCoord5);
	} else {
		// never gets here
	}
#endif
	return shadow;
}
#endif // SHADOW_MAPPING_POINT_LIGHT

#ifdef LIGHT_IS_SPOTLIGHT
uniform vec3 u_lightDir;
uniform float u_spotLightMaxCos;
#endif

// #ifdef HAS_DOOM3_ALPHATEST
// uniform float u_alphaTestValue;
// #endif

void main() {
#if 0
	gl_FragColor.rgb = v_vertNormal;
	return;
#endif
	// calculate texcoord
#ifdef HAS_HEIGHT_MAP
    vec3 eyeDirNormalized = normalize(v_tbnEyeDir);
#ifdef USE_RELIEF_MAPPING
	// relief mapping
	vec2 texCoord = ReliefMappingRayCast(gl_TexCoord[0].xy,eyeDirNormalized);
#else
	// simple height mapping
    vec4 offset = texture2D(heightMap, gl_TexCoord[0].xy);
	offset = offset * 0.05 - 0.02;
	vec2 texCoord = offset.xy * eyeDirNormalized.xy +  gl_TexCoord[0].xy;   
#endif
#else
	vec2 texCoord = gl_TexCoord[0].st;
#endif 
	// calculate light direction and distance to current pixel
	vec3 lightToVert = u_lightOrigin - v_vertXYZ;
	float distance = length(lightToVert);
	if(distance > u_lightRadius) {
		// pixel is too far from the ligh
		return;
	}
    vec3 lightDirection = normalize(lightToVert);
    
#ifdef LIGHT_IS_SPOTLIGHT
	float spotDOT = dot(lightDirection,u_lightDir);
	if(-spotDOT < u_spotLightMaxCos) {
		return;
	}
#endif

#ifdef HAS_BUMP_MAP
	vec3 bumpMapNormal = texture2D (bumpMap, texCoord);
	bumpMapNormal = (bumpMapNormal - 0.5) * 2.0;
	v_vertNormal = tbnMat * bumpMapNormal;
#endif    
    // calculate the diffuse value based on light angle	
    float angleFactor = dot(v_vertNormal, lightDirection);
    if(angleFactor < 0.0) {
		// light is behind the surface
		return;
    }
#ifdef DEBUG_IGNOREANGLEFACTOR
    angleFactor = 1.0;
#endif // DEBUG_IGNOREANGLEFACTOR  
	//  apply distnace scale
  	float distanceFactor = 1.0 - distance / u_lightRadius;
  	
#ifdef DEBUG_IGNOREDISTANCEFACTOR
  	distanceFactor = 1.0;
#endif // DEBUG_IGNOREDISTANCEFACTOR

#ifdef SHADOW_MAPPING_POINT_LIGHT
	vec4 lightWorld = (u_entityMatrix) * vec4(u_lightOrigin,1);
	vec4 vertWorld = (u_entityMatrix) * vec4(v_vertXYZ,1);
	vec4 lightToVert_world = lightWorld - vertWorld;
	float shadow = computeShadow(lightToVert_world.xyz);
#else
	float shadow = 1.0;
#endif
	vec4 textureColor = texture2D (colorMap, texCoord);
// #ifdef HAS_DOOM3_ALPHATEST
// 	if(textureColor.a < u_alphaTestValue)
// 	{
// 		discard;
// 	}
// #endif
	// calculate the final color
	gl_FragColor = textureColor * angleFactor * distanceFactor * shadow;
}