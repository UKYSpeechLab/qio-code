/*
============================================================================
Copyright (C) 2016 V.

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
// img_webpp.cpp - webpp image format
#include "img_local.h"
#include <api/vfsAPI.h>
#include <qcommon/q_shared.h>

#include <webp/decode.h>

bool IMG_LoadWEBP(const char *fname, const byte *buffer, const u32 bufferLen, byte **pic, u32 *width, u32 *height) {
	int h, w;
	if( !WebPGetInfo( buffer, bufferLen, &w, &h ) )
		return true;
	*height = h;
	*width = w;

	byte *out = *pic = (byte*)malloc(w*h*4);
	if( !WebPDecodeRGBAInto( buffer, bufferLen, out, w*h*4, w*4 ) ) {
		free(out);
		return true;
	}
	*pic = out;
	return false;
}