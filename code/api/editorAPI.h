/*
============================================================================
Copyright (C) 2014 V.

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
// editorAPI.h

#ifndef __EDITOR_API_H__
#define __EDITOR_API_H__

#include "iFaceBase.h"

#define EDITOR_API_IDENTSTR "EditorAPI0001"

class editorAPI_i : public iFaceBase_i {
public:

	virtual bool runEditor() = 0;
	virtual bool initEditor() = 0;
	virtual void shutdownEditor() = 0;
};

extern editorAPI_i *g_editor;

#endif // __EDITOR_API_H__
