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
// BaseEntity.h - base class for all entities

#ifndef __BASEENTITY_H__
#define __BASEENTITY_H__

#include "../g_classes.h" // DECLARE_CLASS, etc

class BaseEntity {
	struct entityState_s *_myEntityState; // this is NULL only for players !!! (they are using playerState_s instead)
protected:
	// entity's edict, set once during entity allocation
	struct edict_s *myEdict;
	// bullet physics object
	class btRigidBody *body;
	// simplified model for collision detection
	class cMod_i *cmod;
public:
	BaseEntity();
	virtual ~BaseEntity();

	DECLARE_CLASS( BaseEntity );

	virtual void setKeyValue(const char *key, const char *value);

	virtual void setOrigin(const class vec3_c &newXYZ);
	virtual void setAngles(const class vec3_c &newAngles);
	const class vec3_c &getOrigin() const;
	const class vec3_c &getAngles() const;

	virtual void runFrame() {

	}
	virtual void debugDraw(class rDebugDrawer_i *dd) {

	}
};

void BE_SetForcedEdict(edict_s *nfe);

#endif // __BASEENTITY_H__