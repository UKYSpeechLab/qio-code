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

/*

  QERadiant Undo/Redo


basic setup:

<-g_undolist---------g_lastundo> <---map data---> <-g_lastredo---------g_redolist->


  undo/redo on the world_entity is special, only the epair changes are remembered
  and the world entity never gets deleted.

  FIXME: maybe reset the Undo system at map load
		 maybe also reset the entityId at map load
*/

#include <stdafx.h>
#include "Radiant.h"
#include "qe3.h"
#include <api/entityDeclAPI.h>

struct undo_s
{
	double time;				//time operation was performed
	int id;						//every undo has an unique id
	int done;					//true when undo is build
	const char *operation;		//name of the operation
	edBrush_c brushlist;			//deleted brushes
	entity_s entitylist;		//deleted entities
	struct undo_s *prev, *next;	//next and prev undo in list
};

undo_s *g_undolist;						//first undo in the list
undo_s *g_lastundo;						//last undo in the list
undo_s *g_redolist;						//first redo in the list
undo_s *g_lastredo;						//last undo in list
int g_undoMaxSize = 64;					//maximum number of undos
int g_undoSize = 0;						//number of undos in the list
int g_undoMaxMemorySize = 2*1024*1024;	//maximum undo memory (default 2 MB)
int g_undoMemorySize = 0;				//memory size of undo buffer
int g_undoId = 1;						//current undo ID (zero is invalid id)
int g_redoId = 1;						//current redo ID (zero is invalid id)

int Undo_MemorySize()
{
	/*
	int size;
	undo_s *undo;
	edBrush_c *pBrush;
	entity_s *pEntity;

	size = 0;
	for (undo = g_undolist; undo; undo = undo->next)
	{
		for (pBrush = undo->brushlist.next ; pBrush != NULL && pBrush != &undo->brushlist ; pBrush = pBrush->next)
		{
			size += Brush_MemorySize(pBrush);
		}
		for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = pEntity->next)
		{
			size += Entity_MemorySize(pEntity);
		}
		size += sizeof(undo_s);
	}
	return size;
	*/
	return g_undoMemorySize;
}

void Undo_ClearRedo()
{
	undo_s *redo, *nextredo;
	edBrush_c *pBrush, *pNextBrush;
	entity_s *pEntity, *pNextEntity;

	for (redo = g_redolist; redo; redo = nextredo)
	{
		nextredo = redo->next;
		for (pBrush = redo->brushlist.next ; pBrush != NULL && pBrush != &redo->brushlist ; pBrush = pNextBrush)
		{
			pNextBrush = pBrush->next;
			Brush_Free(pBrush);
		}
		for (pEntity = redo->entitylist.getNextEntity(); pEntity != NULL && pEntity != &redo->entitylist; pEntity = pNextEntity)
		{
			pNextEntity = pEntity->getNextEntity();
			delete pEntity;
		}
		free(redo);
	}
	g_redolist = NULL;
	g_lastredo = NULL;
	g_redoId = 1;
}
// Clears the undo buffer.
void Undo_Clear()
{
	undo_s *undo, *nextundo;
	edBrush_c *pBrush, *pNextBrush;
	entity_s *pEntity, *pNextEntity;

	Undo_ClearRedo();
	for (undo = g_undolist; undo; undo = nextundo)
	{
		nextundo = undo->next;
		for (pBrush = undo->brushlist.next ; pBrush != NULL && pBrush != &undo->brushlist ; pBrush = pNextBrush)
		{
			pNextBrush = pBrush->next;
			g_undoMemorySize -= Brush_MemorySize(pBrush);
			Brush_Free(pBrush);
		}
		for (pEntity = undo->entitylist.getNextEntity(); pEntity != NULL && pEntity != &undo->entitylist; pEntity = pNextEntity)
		{
			pNextEntity = pEntity->getNextEntity();
			g_undoMemorySize -= pEntity->getMemorySize();
			delete pEntity;
		}
		g_undoMemorySize -= sizeof(undo_s);
		free(undo);
	}
	g_undolist = NULL;
	g_lastundo = NULL;
	g_undoSize = 0;
	g_undoMemorySize = 0;
	g_undoId = 1;
}

void Undo_SetMaxSize(int size)
{
	Undo_Clear();
	if (size < 1) g_undoMaxSize = 1;
	else g_undoMaxSize = size;
}

int Undo_GetMaxSize()
{
	return g_undoMaxSize;
}

void Undo_SetMaxMemorySize(int size)
{
	Undo_Clear();
	if (size < 1024) g_undoMaxMemorySize = 1024;
	else g_undoMaxMemorySize = size;
}

int Undo_GetMaxMemorySize()
{
	return g_undoMaxMemorySize;
}

void Undo_FreeFirstUndo()
{
	undo_s *undo;
	edBrush_c *pBrush, *pNextBrush;
	entity_s *pEntity, *pNextEntity;

	//remove the oldest undo from the undo buffer
	undo = g_undolist;
	g_undolist = g_undolist->next;
	g_undolist->prev = NULL;
	//
	for (pBrush = undo->brushlist.next ; pBrush != NULL && pBrush != &undo->brushlist ; pBrush = pNextBrush)
	{
		pNextBrush = pBrush->next;
		g_undoMemorySize -= Brush_MemorySize(pBrush);
		Brush_Free(pBrush);
	}
	for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		g_undoMemorySize -= pEntity->getMemorySize();
		delete pEntity;
	}
	g_undoMemorySize -= sizeof(undo_s);
	free(undo);
	g_undoSize--;
}

void Undo_GeneralStart(const char *operation)
{
	undo_s *undo;
	edBrush_c *pBrush;
	entity_s *pEntity;


	if (g_lastundo)
	{
		if (!g_lastundo->done)
		{
			Sys_Printf("Undo_Start: WARNING last undo not finished.\n");
		}
	}

	undo = (undo_s *) malloc(sizeof(undo_s));
	if (!undo) return;
	memset(undo, 0, sizeof(undo_s));
	undo->brushlist.next = &undo->brushlist;
	undo->brushlist.prev = &undo->brushlist;
	undo->entitylist.next = &undo->entitylist;
	undo->entitylist.prev = &undo->entitylist;
	if (g_lastundo) g_lastundo->next = undo;
	else g_undolist = undo;
	undo->prev = g_lastundo;
	undo->next = NULL;
	g_lastundo = undo;
	
	undo->time = Sys_DoubleTime();
	//
	if (g_undoId > g_undoMaxSize * 2) g_undoId = 1;
	if (g_undoId <= 0) g_undoId = 1;
	undo->id = g_undoId++;
	undo->done = false;
	undo->operation = operation;
	//reset the undo IDs of all brushes using the new ID
	for (pBrush = active_brushes.next; pBrush != NULL && pBrush != &active_brushes; pBrush = pBrush->next)
	{
		if (pBrush->undoId == undo->id)
		{
			pBrush->undoId = 0;
		}
	}
	for (pBrush = selected_brushes.next; pBrush != NULL && pBrush != &selected_brushes; pBrush = pBrush->next)
	{
		if (pBrush->undoId == undo->id)
		{
			pBrush->undoId = 0;
		}
	}
	//reset the undo IDs of all entities using thew new ID
	for (pEntity = entities.next; pEntity != NULL && pEntity != &entities; pEntity = pEntity->next)
	{
		if (pEntity->undoId == undo->id)
		{
			pEntity->undoId = 0;
		}
	}
	g_undoMemorySize += sizeof(undo_s);
	g_undoSize++;
	//undo buffer is bound to a max
	if (g_undoSize > g_undoMaxSize)
	{
		Undo_FreeFirstUndo();
	}
}

int Undo_BrushInUndo(undo_s *undo, edBrush_c *brush)
{
	edBrush_c *b;

	for (b = undo->brushlist.next; b != &undo->brushlist; b = b->next)
	{
		if (b == brush) return true;
	}
	return false;
}

int Undo_EntityInUndo(undo_s *undo, entity_s *ent)
{
	entity_s *e;

	for (e = undo->entitylist.next; e != &undo->entitylist; e = e->next)
	{
		if (e == ent) return true;
	}
	return false;
}

void Undo_Start(const char *operation)
{
	Undo_ClearRedo();
	Undo_GeneralStart(operation);
}

void Undo_AddBrush(edBrush_c *pBrush)
{
	if (!g_lastundo)
	{
		Sys_Printf("Undo_AddBrushList: no last undo.\n");
		return;
	}
	if (g_lastundo->entitylist.getNextEntity() != &g_lastundo->entitylist)
	{
		Sys_Printf("Undo_AddBrushList: WARNING adding brushes after entity.\n");
	}
	//if the brush is already in the undo
	if (Undo_BrushInUndo(g_lastundo, pBrush))
		return;
	//clone the brush
	edBrush_c* pClone = pBrush->fullClone();
	// V: don't store entity pointer because entity might be fried earlier...
	pClone->owner = 0;
	//save the ID of the owner entity
	pClone->ownerId = pBrush->owner->entityId;
	//save the old undo ID for previous undos
	pClone->undoId = pBrush->undoId;
	Brush_AddToList (pClone, &g_lastundo->brushlist);
	//
	g_undoMemorySize += Brush_MemorySize(pClone);
}

void Undo_AddBrushList(edBrush_c *brushlist)
{
	edBrush_c *pBrush;

	if (!g_lastundo)
	{
		Sys_Printf("Undo_AddBrushList: no last undo.\n");
		return;
	}
	if (g_lastundo->entitylist.getNextEntity() != &g_lastundo->entitylist)
	{
		Sys_Printf("Undo_AddBrushList: WARNING adding brushes after entity.\n");
	}
	//copy the brushes to the undo
	for (pBrush = brushlist->next ; pBrush != NULL && pBrush != brushlist; pBrush=pBrush->next)
	{
		//if the brush is already in the undo
		if (Undo_BrushInUndo(g_lastundo, pBrush))
			continue;
		// do we need to store this brush's entity in the undo?
		// if it's a fixed size entity, the brush that reprents it is not really relevant, it's used for selecting and moving around
		// what we want to store for undo is the owner entity, epairs and origin/angle stuff
		//++timo FIXME: if the entity is not fixed size I don't know, so I don't do it yet
		if (pBrush->owner->getEntityClass()->isFixedSize() == 1)
			Undo_AddEntity( pBrush->owner );
		//clone the brush
		edBrush_c* pClone = pBrush->fullClone();
		// V: HACK - don't display brushes stored in undo in renderer
		pClone->freeBrushRenderData();
		// V: don't store entity pointer because entity might be fried earlier...
		pClone->owner = 0;
		//save the ID of the owner entity
		pClone->ownerId = pBrush->owner->entityId;
		//save the old undo ID from previous undos
		pClone->undoId = pBrush->undoId;
		Brush_AddToList (pClone, &g_lastundo->brushlist);
		//
		g_undoMemorySize += Brush_MemorySize(pClone);
	}
}

void Undo_EndBrush(edBrush_c *pBrush)
{
	if (!g_lastundo)
	{
		//Sys_Printf("Undo_End: no last undo.\n");
		return;
	}
	if (g_lastundo->done)
	{
		//Sys_Printf("Undo_End: last undo already finished.\n");
		return;
	}
	pBrush->undoId = g_lastundo->id;
}

void Undo_EndBrushList(edBrush_c *brushlist)
{
	if (!g_lastundo)
	{
		//Sys_Printf("Undo_End: no last undo.\n");
		return;
	}
	if (g_lastundo->done)
	{
		//Sys_Printf("Undo_End: last undo already finished.\n");
		return;
	}
	for (edBrush_c* pBrush = brushlist->next; pBrush != NULL && pBrush != brushlist; pBrush=pBrush->next)
	{
		pBrush->undoId = g_lastundo->id;
	}
}

void Undo_AddEntity(entity_s *entity)
{
	entity_s* pClone;

	if (!g_lastundo)
	{
		Sys_Printf("Undo_AddEntity: no last undo.\n");
		return;
	}
	//if the entity is already in the undo
	if (Undo_EntityInUndo(g_lastundo, entity))
		return;
	//clone the entity
	pClone = Entity_Clone(entity);
	//NOTE: Entity_Clone adds the entity to the entity list
	//		so we remove it from that list here
	Entity_RemoveFromList(pClone);
	//save the old undo ID for previous undos
	pClone->undoId = entity->undoId;
	//save the entity ID (we need a full clone)
	pClone->entityId = entity->entityId;
	//
	Entity_AddToList(pClone, &g_lastundo->entitylist);
	//
	g_undoMemorySize += pClone->getMemorySize();
}

void Undo_EndEntity(entity_s *entity)
{
	if (!g_lastundo)
	{
		//Sys_Printf("Undo_End: no last undo.\n");
		return;
	}
	if (g_lastundo->done)
	{
		//Sys_Printf("Undo_End: last undo already finished.\n");
		return;
	}
	if (entity == world_entity)
	{
		//Sys_Printf("Undo_AddEntity: undo on world entity.\n");
		//NOTE: we never delete the world entity when undoing an operation
		//		we only transfer the epairs
		return;
	}
	entity->undoId = g_lastundo->id;
}

void Undo_End()
{
	if (!g_lastundo)
	{
		//Sys_Printf("Undo_End: no last undo.\n");
		return;
	}
	if (g_lastundo->done)
	{
		//Sys_Printf("Undo_End: last undo already finished.\n");
		return;
	}
	g_lastundo->done = true;

	//undo memory size is bound to a max
	while (g_undoMemorySize > g_undoMaxMemorySize)
	{
		//always keep one undo
		if (g_undolist == g_lastundo) break;
		Undo_FreeFirstUndo();
	}
	//
	//Sys_Printf("undo size = %d, undo memory = %d\n", g_undoSize, g_undoMemorySize);
}

void Undo_Undo()
{
	undo_s *undo, *redo;
	edBrush_c *pBrush, *pNextBrush;
	entity_s *pEntity, *pNextEntity, *pUndoEntity;

	if (!g_lastundo)
	{
		Sys_Printf("Nothing left to undo.\n");
		return;
	}
	if (!g_lastundo->done)
	{
		Sys_Printf("Undo_Undo: WARNING: last undo not yet finished!\n");
	}
	// get the last undo
	undo = g_lastundo;
	if (g_lastundo->prev) g_lastundo->prev->next = NULL;
	else g_undolist = NULL;
	g_lastundo = g_lastundo->prev;

	//allocate a new redo
	redo = (undo_s *) malloc(sizeof(undo_s));
	if (!redo) return;
	memset(redo, 0, sizeof(undo_s));
	redo->brushlist.next = &redo->brushlist;
	redo->brushlist.prev = &redo->brushlist;
	redo->entitylist.next = &redo->entitylist;
	redo->entitylist.prev = &redo->entitylist;
	if (g_lastredo) g_lastredo->next = redo;
	else g_redolist = redo;
	redo->prev = g_lastredo;
	redo->next = NULL;
	g_lastredo = redo;
	redo->time = Sys_DoubleTime();
	redo->id = g_redoId++;
	redo->done = true;
	redo->operation = undo->operation;

	//reset the redo IDs of all brushes using the new ID
	for (pBrush = active_brushes.next; pBrush != NULL && pBrush != &active_brushes; pBrush = pBrush->next)
	{
		if (pBrush->redoId == redo->id)
		{
			pBrush->redoId = 0;
		}
	}
	for (pBrush = selected_brushes.next; pBrush != NULL && pBrush != &selected_brushes; pBrush = pBrush->next)
	{
		if (pBrush->redoId == redo->id)
		{
			pBrush->redoId = 0;
		}
	}
	//reset the redo IDs of all entities using thew new ID
	for (pEntity = entities.getNextEntity(); pEntity != NULL && pEntity != &entities; pEntity = pEntity->getNextEntity())
	{
		if (pEntity->redoId == redo->id)
		{
			pEntity->redoId = 0;
		}
	}

	// remove current selection
	Select_Deselect();
	// move "created" brushes to the redo
	for (pBrush = active_brushes.next; pBrush != NULL && pBrush != &active_brushes; pBrush=pNextBrush)
	{
		pNextBrush = pBrush->next;
		if (pBrush->undoId == undo->id)
		{
			//Brush_Free(pBrush);
			//move the brush to the redo
			Brush_RemoveFromList(pBrush);
			Brush_AddToList(pBrush, &redo->brushlist);
			//make sure the ID of the owner is stored
			pBrush->ownerId = pBrush->owner->entityId;
			//unlink the brush from the owner entity
			Entity_UnlinkBrush(pBrush);
		}
	}
	// move "created" entities to the redo
	for (pEntity = entities.next; pEntity != NULL && pEntity != &entities; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		if (pEntity->undoId == undo->id)
		{
			// check if this entity is in the undo
			for (pUndoEntity = undo->entitylist.getNextEntity(); pUndoEntity != NULL && pUndoEntity != &undo->entitylist; pUndoEntity = pUndoEntity->getNextEntity())
			{
				// move brushes to the undo entity
				if (pUndoEntity->entityId == pEntity->entityId)
				{
					pUndoEntity->brushes.next = pEntity->brushes.next;
					pUndoEntity->brushes.prev = pEntity->brushes.prev;
					pEntity->brushes.next = &pEntity->brushes;
					pEntity->brushes.prev = &pEntity->brushes;
				}
			}
			//
			//move the entity to the redo
			Entity_RemoveFromList(pEntity);
			Entity_AddToList(pEntity, &redo->entitylist);
		}
	}
	// add the undo entities back into the entity list
	for (pEntity = undo->entitylist.getNextEntity(); pEntity != NULL && pEntity != &undo->entitylist; pEntity = undo->entitylist.getNextEntity())
	{
		g_undoMemorySize -= pEntity->getMemorySize();
		//if this is the world entity
		if (pEntity->entityId == world_entity->entityId)
		{
			//set back the original epairs
			world_entity->setKeyValues(pEntity->getKeyValues());
			// free the world_entity clone that stored the epairs
			delete pEntity;
		}
		else
		{
			Entity_RemoveFromList(pEntity);
			Entity_AddToList(pEntity, &entities);
			pEntity->redoId = redo->id;
		}
	}
	// add the undo brushes back into the selected brushes
	for (pBrush = undo->brushlist.next; pBrush != NULL && pBrush != &undo->brushlist; pBrush = undo->brushlist.next)
	{
		g_undoMemorySize -= Brush_MemorySize(pBrush);
		Brush_RemoveFromList(pBrush);
    	Brush_AddToList(pBrush, &active_brushes);
		for (pEntity = entities.next; pEntity != NULL && pEntity != &entities; pEntity = pNextEntity)
		{
			if (pEntity->entityId == pBrush->ownerId)
			{
				pEntity->linkBrush(pBrush);
				break;
			}
		}
		//if the brush is not linked then it should be linked into the world entity
		if (pEntity == NULL || pEntity == &entities)
		{
			world_entity->linkBrush(pBrush);
		}
		//build the brush
		Brush_Build(pBrush);
		Select_Brush(pBrush);
		pBrush->redoId = redo->id;
    }
	//
	Sys_Printf("%s undone.\n", undo->operation);
	// free the undo
	g_undoMemorySize -= sizeof(undo_s);
	free(undo);
	g_undoSize--;
	g_undoId--;
	if (g_undoId <= 0) g_undoId = 2 * g_undoMaxSize;
	//
    g_bScreenUpdates = true; 
    Sys_UpdateWindows(W_ALL);
}

void Undo_Redo()
{
	undo_s *redo;
	edBrush_c *pBrush, *pNextBrush;
	entity_s *pEntity, *pNextEntity, *pRedoEntity;

	if (!g_lastredo)
	{
		Sys_Printf("Nothing left to redo.\n");
		return;
	}
	if (g_lastundo)
	{
		if (!g_lastundo->done)
		{
			Sys_Printf("WARNING: last undo not finished.\n");
		}
	}
	// get the last redo
	redo = g_lastredo;
	if (g_lastredo->prev) g_lastredo->prev->next = NULL;
	else g_redolist = NULL;
	g_lastredo = g_lastredo->prev;
	//
	Undo_GeneralStart(redo->operation);
	// remove current selection
	Select_Deselect();
	// move "created" brushes back to the last undo
	for (pBrush = active_brushes.next; pBrush != NULL && pBrush != &active_brushes; pBrush = pNextBrush)
	{
		pNextBrush = pBrush->next;
		if (pBrush->redoId == redo->id)
		{
			//move the brush to the undo
			Brush_RemoveFromList(pBrush);
			Brush_AddToList(pBrush, &g_lastundo->brushlist);
			g_undoMemorySize += Brush_MemorySize(pBrush);
			pBrush->ownerId = pBrush->owner->entityId;
			Entity_UnlinkBrush(pBrush);
		}
	}
	// move "created" entities back to the last undo
	for (pEntity = entities.next; pEntity != NULL && pEntity != &entities; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		if (pEntity->redoId == redo->id)
		{
			// check if this entity is in the redo
			for (pRedoEntity = redo->entitylist.next; pRedoEntity != NULL && pRedoEntity != &redo->entitylist; pRedoEntity = pRedoEntity->next)
			{
				// move brushes to the redo entity
				if (pRedoEntity->entityId == pEntity->entityId)
				{
					pRedoEntity->brushes.next = pEntity->brushes.next;
					pRedoEntity->brushes.prev = pEntity->brushes.prev;
					pEntity->brushes.next = &pEntity->brushes;
					pEntity->brushes.prev = &pEntity->brushes;
				}
			}
			//
			//move the entity to the redo
			Entity_RemoveFromList(pEntity);
			Entity_AddToList(pEntity, &g_lastundo->entitylist);
			g_undoMemorySize += pEntity->getMemorySize();
		}
	}
	// add the undo entities back into the entity list
	for (pEntity = redo->entitylist.next; pEntity != NULL && pEntity != &redo->entitylist; pEntity = redo->entitylist.next)
	{
		//if this is the world entity
		if (pEntity->entityId == world_entity->entityId)
		{
			//set back the original epairs
			world_entity->setKeyValues(pEntity->getKeyValues());
			//free the world_entity clone that stored the epairs
			delete pEntity;
		}
		else
		{
			Entity_RemoveFromList(pEntity);
			Entity_AddToList(pEntity, &entities);
		}
	}
	// add the redo brushes back into the selected brushes
	for (pBrush = redo->brushlist.next; pBrush != NULL && pBrush != &redo->brushlist; pBrush = redo->brushlist.next)
	{
		Brush_RemoveFromList(pBrush);
    	Brush_AddToList(pBrush, &active_brushes);
		for (pEntity = entities.next; pEntity != NULL && pEntity != &entities; pEntity = pNextEntity)
		{
			if (pEntity->entityId == pBrush->ownerId)
			{
				pEntity->linkBrush(pBrush);
				break;
			}
		}
		//if the brush is not linked then it should be linked into the world entity
		if (pEntity == NULL || pEntity == &entities)
		{
			world_entity->linkBrush(pBrush);
		}
		//build the brush
		//Brush_Build(pBrush);
		Select_Brush(pBrush);
    }
	//
	Undo_End();
	//
	Sys_Printf("%s redone.\n", redo->operation);
	//
	g_redoId--;
	// free the undo
	free(redo);
	//
    g_bScreenUpdates = true; 
    Sys_UpdateWindows(W_ALL);
}


int Undo_RedoAvailable()
{
	if (g_lastredo) return true;
	return false;
}


int Undo_UndoAvailable()
{
	if (g_lastundo)
	{
		if (g_lastundo->done)
			return true;
	}
	return false;
}
