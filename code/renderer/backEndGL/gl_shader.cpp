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
// gl_shader.cpp - GLSL shaders for openGL backend
#include "gl_shader.h"
#include <shared/array.h>
#include <api/coreAPI.h>
#include <api/vfsAPI.h>
#include <api/rbAPI.h>
#include <api/rAPI.h>
#include <api/materialSystemAPI.h>
#include <shared/autoCvar.h>
#include <api/rLightAPI.h> // only for debug scene output
#include <math/vec3.h> // only for debug scene output

#include <time.h>

static aCvar_c gl_saveGLSLErrorLogToFile("gl_saveGLSLErrorLogToFile","1");

void GL_WriteGLSLErrorReport(const char *errorText, const char *shaderText) {
	
	time_t rawtime;
	struct tm * timeinfo;

	//time ( &rawtime );
	//timeinfo = localtime ( 0 );
	//printf ( "Current local time and date: %s", asctime (timeinfo) );
	char buffer[128] = { 0 };
	//strftime (buffer,sizeof(buffer),"%D_%T",timeinfo);
	str timeStr = buffer;
//	timeStr.replaceCharacter(':','_');
	timeStr.replaceCharacter(' ','_');
	str baseName = "glslShaderError_";
	baseName.append(timeStr);
	str errorFile = baseName;
	errorFile.append("_error.txt");
	str sourceFile = baseName;
	sourceFile.append("_source.txt");

	fileHandle_t handle = 0;
	g_vfs->FS_FOpenFile(errorFile,&handle,FS_WRITE);
	if(handle) {
		str errorTextExtended = "";
		if(rb) {
			errorTextExtended.append("\n");
			errorTextExtended.append("// ============\n");
			errorTextExtended.append("// GPU INFO\n");
			errorTextExtended.append("// ============\n");
			const char *gpuInfo = rb->getBackendInfoStr();
			if(gpuInfo) {
				errorTextExtended.append(gpuInfo);
			}
		}

		errorTextExtended.append("// ============\n");
		errorTextExtended.append("// SCENE INFO\n");
		errorTextExtended.append("// ============\n");
		errorTextExtended.append("Mapname: ");
		errorTextExtended.append(rf->getLoadedMapName());
		errorTextExtended.append("\n");
		// dump cached materials
		if(g_ms) {
			u32 totalMats = g_ms->getNumAllocatedMaterials();
			errorTextExtended.append(va("Total %i materials cached.\n",totalMats));
			for(u32 i = 0; i < totalMats; i++) {
				const char *matName = g_ms->getAllocatedMaterialName(i);
				errorTextExtended.append(va("\tMaterial %i of %i is %s\n",i,totalMats,matName));
			}
		}
		// dump cached models
		if(rf) {
			u32 totalMods = rf->getNumAllocatedModels();
			errorTextExtended.append(va("Total %i models cached.\n",totalMods));
			for(u32 i = 0; i < totalMods; i++) {
				const char *modName = rf->getAllocatedModelName(i);
				errorTextExtended.append(va("\tModel %i of %i is %s\n",i,totalMods,modName));
			}
		}
		// dump allocated lights
		if(rf) {
			u32 totalLights = rf->getNumAllocatedLights();
			errorTextExtended.append(va("Total %i lights allocated.\n",totalLights));
			for(u32 i = 0; i < totalLights; i++) {
				rLightAPI_i *light = rf->getLight(i);
				const vec3_c &at = light->getOrigin();
				float radius = light->getRadius();
				const char *typeStr;
				if(light->getLightType() == LT_POINT)
					typeStr = "Point";
				else if(light->getLightType() == LT_SPOTLIGHT)
					typeStr = "Spot";
				else 
					typeStr = "Unknown";

				//const char *modName = rf->getAllocatedModelName(i);
				errorTextExtended.append(va("\tLight %i of %i is at %f %f %f, radius %f, type %s\n",
					i,totalLights,at.x,at.y,at.z,radius,typeStr));
			}
		}
		errorTextExtended.append("\n");

		errorTextExtended.append("// ============\n");
		errorTextExtended.append("// GLSL SHADER ERROR\n");
		errorTextExtended.append("// ============\n");
		errorTextExtended.append(errorText);
		g_vfs->FS_Write(errorTextExtended,strlen(errorTextExtended),handle);
		g_vfs->FS_FCloseFile(handle);
	}
	g_vfs->FS_FOpenFile(sourceFile,&handle,FS_WRITE);
	if(handle) {
		g_vfs->FS_Write(shaderText,strlen(shaderText),handle);
		g_vfs->FS_FCloseFile(handle);
	}
}

bool GL_CompileShaderProgram(GLuint handle, const char *source) {
	glShaderSourceARB(handle, 1,(const GLcharARB**) &source, 0);
	glCompileShaderARB(handle);
	int status;
	glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if(status == 0) {
		int logLen = 0;
		glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen);
		if(logLen > 0) {
			char *tmp = (char*)malloc(logLen+1);
			glGetInfoLogARB(handle, logLen, 0, tmp);
			g_core->RedWarning(tmp);
			if(gl_saveGLSLErrorLogToFile.getInt()) {
				GL_WriteGLSLErrorReport(tmp,source);
			}
			free(tmp);
		}
		return true; // error
	}
	return false;
}

bool P_AppendFileTextToString(str &out, const char *fname) {
	char *fileData;
	int len = g_vfs->FS_ReadFile(fname,(void**)&fileData);
	if(fileData == 0) {
		return true; // cannot open
	}
	out.append(fileData);
	g_vfs->FS_FreeFile(fileData);
	return false;
}
const char *P_SkipWhiteSpaces(const char *in) {
	while(iswspace(*in)) {
		in++;
	}
	return in;
}
const char *P_GetToken(str &out, const char *in) {
	in = P_SkipWhiteSpaces(in);
	if(*in == '"') {
		in++;
		const char *start = in;
		while(*in != '"') {
			in++;
		}
		out.setFromTo(start,in);
		in++;
	} else {
		const char *start = in;
		while(iswspace(*in)==false) {
			in++;
		}
		out.setFromTo(start,in);
	}
	return in;
}
bool P_LoadAndPreprocessFileIncludes(str &out, const char *fname) {
	str rawFile;
	if(P_AppendFileTextToString(rawFile,fname)) {
		return true;
	}
	const char *p = rawFile.c_str();
	const char *start = p;
	const char *stop = 0;
	int includeStringLen = strlen("#include");
	while(*p) {
		if(!strnicmp(p,"#include",includeStringLen)) {
			stop = p;
			p += includeStringLen;
			// get the name of file that must be included
			str includedFileName;
			p = P_GetToken(includedFileName,p);
			// load incldued file text
			str includeFileText;
			if(P_LoadAndPreprocessFileIncludes(includeFileText,includedFileName)) {
				str fullPath = fname;
				fullPath.toDir();
				fullPath.append(includedFileName);
				if(P_LoadAndPreprocessFileIncludes(includeFileText,fullPath)) {
					g_core->RedWarning("LoadAndPreprocessFileIncludes: couldn't load file %s\n",includedFileName.c_str());
				}
			}
			out.append(start,stop);
			// add included file text
			out.append("\n");
			out.append(includeFileText);
			out.append("\n");
			start = p;
		}
		p++;
	}
	out.append(start);

	return false;
}

arraySTD_c<glShader_c*> gl_shaders;
static glShader_c *GL_FindShader(const char *baseName, const glslPermutationFlags_s &permutations) {
	for(u32 i = 0; i < gl_shaders.size(); i++) {
		glShader_c *s = gl_shaders[i];
		if(!stricmp(baseName,s->getName())) {
			if(!memcmp(&s->getPermutations(),&permutations,sizeof(glslPermutationFlags_s))) {
				return s;
			}
		}
	}
	return 0;
}
void GL_AppendPermutationDefinesToString(str &out, const glslPermutationFlags_s &p) {
	if(p.hasLightmap) {
		out.append("#define HAS_LIGHTMAP\n");
	}
	if(p.hasVertexColors) {
		out.append("#define HAS_VERTEXCOLORS\n");
	}
	if(p.hasTexGenEnvironment) {
		out.append("#define HAS_TEXGEN_ENVIROMENT\n");
	}
	if(p.pointLightShadowMapping) {
		out.append("#define SHADOW_MAPPING_POINT_LIGHT\n");
	}
	if(p.hasBumpMap) {
		out.append("#define HAS_BUMP_MAP\n");
	}
	if(p.hasHeightMap) {
		out.append("#define HAS_HEIGHT_MAP\n");
	}
	if(p.useReliefMapping) {
		out.append("#define USE_RELIEF_MAPPING\n");
	}	
	if(p.hasDeluxeMap) {
		out.append("#define HAS_DELUXEMAP\n");
	}
	if(p.hasSpecularMap) {
		out.append("#define HAS_SPECULAR_MAP\n");
	}
	if(p.hasBumpHeightMap) {
		out.append("#define HAS_BUMP_HEIGHTMAP_MAP\n");
	}
	if(p.hasMaterialColor) {
		// extra per-surface material color
		out.append("#define HAS_MATERIAL_COLOR\n");
	}
	if(p.isSpotLight) {
		out.append("#define LIGHT_IS_SPOTLIGHT\n");
	}
	if(p.enableShadowMappingBlur) {
		out.append("#define ENABLE_SHADOW_MAPPING_BLUR\n");
	}
	if(p.useShadowCubeMap) {
		out.append("#define USE_SHADOW_CUBEMAP\n");
	}
	if(p.isTwoSided) {
		out.append("#define MATERIAL_TWO_SIDED\n");
	}
	if(p.spotLightShadowMapping) {
		out.append("#define SHADOW_MAPPING_SPOTLIGHT\n");
	}
	if(p.debug_showSpotLightShadows) {
		out.append("#define DEBUG_SHOW_SPOTLIGHT_SHADOWS\n");
	}
	if(p.debug_showPointLightShadows) {
		out.append("#define DEBUG_SHOW_POINTLIGHT_SHADOWS\n");
	}
	//if(p.hasDoom3AlphaTest) {
	//	out.append("#define HAS_DOOM3_ALPHATEST\n");
	//}
	if(p.debug_ignoreAngleFactor) {
		out.append("#define DEBUG_IGNOREANGLEFACTOR\n");
	}
	if(p.debug_ignoreDistanceFactor) {
		out.append("#define DEBUG_IGNOREDISTANCEFACTOR\n");
	}
	if(p.debug_showSplits) {
		out.append("#define DEBUG_SHADOWMAPPING_SHOW_SPLITS\n");
	}
	if(p.debug_ignoreSpecular) {
		out.append("#define DEBUG_SKIP_SPECULAR\n");
	}
	if(p.bHorizontalPass) {
		out.append("#define HORIZONTAL_PASS\n");
	}
	if(p.hasSunLight) {
		out.append("#define HAS_SUNLIGHT\n");
	}
	if(p.hasDirectionalShadowMapping) {
		out.append("#define HAS_DIRECTIONAL_SHADOW_MAPPING\n");
	}
	if(p.bHasShadowMapLod1) {
		out.append("#define HAS_SHADOWMAP_LOD1\n");
	}
	if(p.bHasShadowMapLod2) {
		out.append("#define HAS_SHADOWMAP_LOD2\n");
	}
	if(p.hasLightColor) {
		out.append("#define HAS_LIGHT_COLOR\n");
	}
	if(p.hasBlendBox) {
		out.append("#define HAS_BLEND_BOX\n");
	}
	if(p.bHasMinShadow) {
		out.append("#define HAS_MIN_SHADOW\n");
	}
}
static glslPermutationFlags_s gl_defaultPermutations;
glShader_c *GL_RegisterShader(const char *baseName, const glslPermutationFlags_s *permutations) {
	if(permutations == 0) {
		permutations = &gl_defaultPermutations;
	}
	// see if the shader is already loaded
	glShader_c *ret = GL_FindShader(baseName,*permutations);
	if(ret) {
		if(ret->isValid())
			return ret;
		return 0;
	}
	// if not, try to load it
	str vertFile = "glsl/";
	vertFile.append(baseName);
	vertFile.append(".vert");
	str fragFile = "glsl/";
	fragFile.append(baseName);
	fragFile.append(".frag");
	ret = new glShader_c;
	ret->permutations = *permutations;
	ret->name = baseName;
	gl_shaders.push_back(ret);
	if(g_vfs->FS_FileExists(fragFile) == false) {
		g_core->RedWarning("GL_RegisterShader: file %s does not exist\n",fragFile.c_str());
		return 0;
	}
	if(g_vfs->FS_FileExists(vertFile) == false) {
		g_core->RedWarning("GL_RegisterShader: file %s does not exist\n",vertFile.c_str());
		return 0;
	}
	str vertexSource;
	// append system #defines
	GL_AppendPermutationDefinesToString(vertexSource,*permutations);
	if(P_LoadAndPreprocessFileIncludes(vertexSource,vertFile)) {
		g_core->RedWarning("GL_RegisterShader: cannot open %s for reading\n",vertFile.c_str());
		return 0;
	}
	str fragmentSource;
	// append system #defines
	GL_AppendPermutationDefinesToString(fragmentSource,*permutations);
	if(P_LoadAndPreprocessFileIncludes(fragmentSource,fragFile)) {
		g_core->RedWarning("GL_RegisterShader: cannot open %s for reading\n",fragFile.c_str());
		return 0;
	}
	if(glCreateShaderObjectARB == 0) {
		g_core->RedWarning("GL_RegisterShader: glCreateShaderObjectARB not available, cannot create shader %s.\n",baseName);
		return 0;
	}
	if(glLinkProgramARB == 0) {
		g_core->RedWarning("GL_RegisterShader: glLinkProgramARB not available, cannot create shader %s.\n",baseName);
		return 0;
	}
	// load separate programs
	// vertex program (.vert file)
	GLuint vertexProgram = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	if(GL_CompileShaderProgram(vertexProgram,vertexSource)) {
		glDeleteShader(vertexProgram);
		return 0;
	}
	// fragment program (.frag file)
	GLuint fragmentProgram = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	if(GL_CompileShaderProgram(fragmentProgram,fragmentSource)) {
		glDeleteShader(vertexProgram);
		glDeleteShader(fragmentProgram);
		return 0;
	}	
	// link vertex and fragment programs to create final shader
	GLhandleARB shader = glCreateProgramObjectARB();
	glAttachObjectARB(shader,vertexProgram);
	glAttachObjectARB(shader,fragmentProgram);
	glLinkProgramARB(shader);
	// check for errors
	int status;
	glGetObjectParameterivARB(shader, GL_OBJECT_LINK_STATUS_ARB, &status);
	if(status == 0) {
		int logLen = 0;
		glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen);
		if(logLen > 0) {
			char *tmp = (char*)malloc(logLen+1);
			glGetInfoLogARB(shader, logLen, 0, tmp);
			g_core->RedWarning(tmp);
			if(gl_saveGLSLErrorLogToFile.getInt()) {
				str source = vertexSource;
				source.append("\n");
				source.append("// FRAGMENT SOURCE NOW\n");
				source.append(fragmentSource);
				GL_WriteGLSLErrorReport(tmp,source);
			}
			free(tmp);
		}
		glDeleteShader(vertexProgram);
		glDeleteShader(fragmentProgram);
		glDeleteProgram(shader);
		return 0;
	}
	ret->handle = shader;
	// precache uniform locations
	ret->sColorMap = glGetUniformLocation(shader,"colorMap");
	ret->sColorMap2 = glGetUniformLocation(shader,"colorMap2");
	ret->sLightMap = glGetUniformLocation(shader,"lightMap");
	ret->sBumpMap = glGetUniformLocation(shader,"bumpMap");
	ret->sHeightMap = glGetUniformLocation(shader,"heightMap");
	ret->sDeluxeMap = glGetUniformLocation(shader,"deluxeMap");
	ret->sSpecularMap = glGetUniformLocation(shader,"specularMap");
	ret->sCubeMap = glGetUniformLocation(shader,"cubeMap");
	ret->sBlendChannelRed = glGetUniformLocation(shader,"blendChannelRed");
	ret->sBlendChannelGreen = glGetUniformLocation(shader,"blendChannelGreen");
	ret->sBlendChannelBlue = glGetUniformLocation(shader,"blendChannelBlue");
	ret->uLightOrigin = glGetUniformLocation(shader,"u_lightOrigin");
	ret->uLightRadius = glGetUniformLocation(shader,"u_lightRadius");
	ret->uViewOrigin = glGetUniformLocation(shader,"u_viewOrigin");
	ret->u_shadowMap[0] = glGetUniformLocation(shader,"shadowMap0");
	ret->u_shadowMap[1] = glGetUniformLocation(shader,"shadowMap1");
	ret->u_shadowMap[2] = glGetUniformLocation(shader,"shadowMap2");
	ret->u_shadowMap[3] = glGetUniformLocation(shader,"shadowMap3");
	ret->u_shadowMap[4] = glGetUniformLocation(shader,"shadowMap4");
	ret->u_shadowMap[5] = glGetUniformLocation(shader,"shadowMap5");
	ret->u_shadowCubeMap = glGetUniformLocation(shader,"shadowCubeMap");
	ret->u_directionalShadowMap = glGetUniformLocation(shader,"directionalShadowMap");
	ret->u_spotLightShadowMap = glGetUniformLocation(shader,"spotLightShadowMap");
	ret->u_materialColor = glGetUniformLocation(shader,"u_materialColor");
	ret->u_entityMatrix = glGetUniformLocation(shader,"u_entityMatrix");
	ret->u_entityRotationMatrix = glGetUniformLocation(shader,"u_entityRotationMatrix");
	ret->u_lightDir = glGetUniformLocation(shader,"u_lightDir");
	ret->u_spotLightMaxCos = glGetUniformLocation(shader,"u_spotLightMaxCos");
	//ret->u_alphaTestValue = glGetUniformLocation(shader,"u_alphaTestValue");
	ret->u_sunDirection = glGetUniformLocation(shader,"u_sunDirection");
	ret->u_sunColor = glGetUniformLocation(shader,"u_sunColor");
	ret->u_blurScale = glGetUniformLocation(shader,"u_blurScale");
	ret->u_averageScreenLuminance = glGetUniformLocation(shader,"u_averageScreenLuminance");
	ret->u_directionalShadowMap_lod1 = glGetUniformLocation(shader,"directionalShadowMap_lod1");
	ret->u_directionalShadowMap_lod2 = glGetUniformLocation(shader,"directionalShadowMap_lod2");
	ret->u_shadowMapLod0Mins = glGetUniformLocation(shader,"u_shadowMapLod0Mins");
	ret->u_shadowMapLod0Maxs = glGetUniformLocation(shader,"u_shadowMapLod0Maxs");
	ret->u_shadowMapLod1Mins = glGetUniformLocation(shader,"u_shadowMapLod1Mins");
	ret->u_shadowMapLod1Maxs = glGetUniformLocation(shader,"u_shadowMapLod1Maxs");
	ret->u_shadowMapSize = glGetUniformLocation(shader,"u_shadowMapSize");
	ret->u_lightColor = glGetUniformLocation(shader,"u_lightColor");
	ret->u_blendBoxMins = glGetUniformLocation(shader,"u_blendBoxMins");
	ret->u_blendBoxMaxs = glGetUniformLocation(shader,"u_blendBoxMaxs");
	ret->u_shininess = glGetUniformLocation(shader,"u_shininess");
	ret->u_minShadow = glGetUniformLocation(shader,"u_minShadow");

	ret->atrTangents = glGetAttribLocation(shader,"atrTangents");
	ret->atrBinormals = glGetAttribLocation(shader,"atrBinormals");

	return ret;
}
void GL_ShutdownGLSLShaders() {
	for(u32 i = 0; i < gl_shaders.size(); i++) {
		delete gl_shaders[i];
	}
	gl_shaders.clear();
}
