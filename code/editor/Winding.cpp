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


#include "stdafx.h"
#include <assert.h>
#include "qe3.h"
#include "winding.h"

#define	BOGUS_RANGE	18000

bool edWinding_t::removePoint(int point)
{
	if (point < 0 || point >= this->size())
		return true;
	points.erase(point);
	return false;
}

edWinding_t *edWinding_t::insertPoint(vec3_t point, int spot)
{
	int i, j;
	edWinding_t *neww;

	if (spot > this->size())
	{
		return 0;
	} //end if
	if (spot < 0)
	{
		return 0;
	} //end if
	neww = new edWinding_t(this->size() + 1);
	for (i = 0, j = 0; i < neww->size(); i++)
	{
		if (i == spot)
		{
			neww->points[i].setXYZ(point);
		}
		else
		{
			neww->points[i] = this->points[j];
			j++;
		}
	}
	return neww;
}

#define	EDGE_LENGTH	0.2

bool edWinding_t::isTiny () const
{
	int		i, j;
	float	len;
	int		edges;

	edges = 0;
	for (i=0 ; i<this->size() ; i++)
	{
		j = i == this->size() - 1 ? 0 : i+1;
		vec3_c delta = this->points[j].getXYZ() - this->points[i].getXYZ();
		len = delta.vectorLength();
		if (len > EDGE_LENGTH)
		{
			if (++edges == 3)
				return false;
		}
	}
	return true;
}

#define WCONVEX_EPSILON		0.2

int edWinding_t::planesConcave(edWinding_t *w1, edWinding_t *w2,
							 const vec3_c &normal1, const vec3_c &normal2,
							 float dist1, float dist2)
{
	int i;

	if (!w1 || !w2) return false;

	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for (i = 0; i < w1->size(); i++)
	{
		if (normal2.dotProduct(w1->points[i]) - dist2 > WCONVEX_EPSILON)
			return true;
	}
	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for (i = 0; i < w2->size(); i++)
	{
		if (normal1.dotProduct(w2->points[i]) - dist1 > WCONVEX_EPSILON)
			return true;
	}

	return false;
}
edWinding_t::edWinding_t() {

}
edWinding_t::edWinding_t(u32 newCount) {
	points.resize(newCount);
}
edWinding_t::edWinding_t(const edPlane_c &p) {
	int		i, x;
	float	max, v;
	vec3_c	org, vright, vup;
	
	// find the major axis

	max = -BOGUS_RANGE;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(p.normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Error ("Winding_BaseForPlane: no axis found");
		
	vup.clear();	
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}


	v = vup.dotProduct(p.normal);
	vup.vectorMA (vup, -v, p.normal);
	vup.normalize();
		
	org = p.normal * p.dist;

	vright.crossProduct (vup, p.normal);
	
	vup *= BOGUS_RANGE;
	vright *= BOGUS_RANGE;


	this->addPointXYZ(org - vright + vup);
	this->addPointXYZ(org + vright + vup);
	this->addPointXYZ(org + vright - vup);
	this->addPointXYZ(org - vright - vup);
		
}
edWinding_t *edWinding_t::cloneWinding() {
	edWinding_t *c = new edWinding_t();
	c->points = points;
	return c;
}
/*
Clips the winding to the plane, returning the new winding on the positive side
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
*/
edWinding_t *edWinding_t::clip(const edPlane_c &split, bool keepon)
{
	float	dists[MAX_POINTS_ON_WINDING];
	int		sides[MAX_POINTS_ON_WINDING];
	int		counts[3];
	float	dot;
	int		i, j;
	float	*p1, *p2;
	vec3_t	mid;
	edWinding_t	*neww;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i=0 ; i<this->size() ; i++)
	{
		dot = this->points[i].dotProduct(split.normal);
		dot -= split.dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (keepon && !counts[0] && !counts[1])
		return this;
		
	if (!counts[0])
	{
		delete (this);
		return NULL;
	}
	if (!counts[1])
		return this;
	
	maxpts = this->size()+4;	// can't use counts[0]+2 because
								// of fp grouping errors
	neww = new edWinding_t ();
		
	for (i=0 ; i<this->size() ; i++)
	{
		p1 = this->points[i];
		
		if (sides[i] == SIDE_ON)
		{
			neww->addPointXYZ(p1);
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			neww->addPointXYZ(p1);
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		// generate a split point
		p2 = this->points[(i+1)%this->size()];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split.normal[j] == 1)
				mid[j] = split.dist;
			else if (split.normal[j] == -1)
				mid[j] = -split.dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		neww->addPointXYZ(mid);
	}
	
	if (neww->size() > maxpts)
		Error ("Winding_Clip: points exceeded estimate");
		
	// free the original winding
	delete (this);
	
	return neww;
}

/*
  split the input winding with the plane
  the input winding stays untouched
*/
void edWinding_t::splitEpsilon (vec3_t normal, double dist, 
				float epsilon, edWinding_t **front, edWinding_t **back)
{
	float	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	float	dot;
	int		i, j;
	float	*p1, *p2;
	vec3_t	mid;
	edWinding_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < this->size(); i++)
	{
		dot = this->points[i].dotProduct(normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	*front = *back = NULL;

	if (!counts[0])
	{
		*back = this->cloneWinding();
		return;
	}
	if (!counts[1])
	{
		*front = this->cloneWinding();
		return;
	}

	maxpts = this->size()+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = new edWinding_t ();
	*back = b = new edWinding_t ();
		
	for (i = 0; i < this->size(); i++)
	{
		p1 = this->points[i];
		
		if (sides[i] == SIDE_ON)
		{
			f->addPointXYZ(p1);
			b->addPointXYZ(p1);
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			f->addPointXYZ(p1);
		}
		if (sides[i] == SIDE_BACK)
		{
			b->addPointXYZ(p1);
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		// generate a split point
		p2 = this->points[(i+1)%this->size()];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		f->addPointXYZ(mid);
		b->addPointXYZ(mid);
	}
	
	if (f->size() > maxpts || b->size() > maxpts)
		Error ("Winding_Clip: points exceeded estimate");
	if (f->size() > MAX_POINTS_ON_WINDING || b->size() > MAX_POINTS_ON_WINDING)
		Error ("Winding_Clip: MAX_POINTS_ON_WINDING");
}

/*
If two windings share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the windings couldn't be merged, or the new winding.
The originals will NOT be freed.

if keep is true no points are ever removed
*/
#define	CONTINUOUS_EPSILON	0.005

edWinding_t *edWinding_t::tryMerge(const edWinding_t *f2, vec3_t planenormal, int keep)
{
	vec3_c p1, p2, p3, p4, back;
	edWinding_t	*newf;
	int			i, j, k, l;
	vec3_c		normal, delta;
	float		dot;
	bool	keep1, keep2;
	

	//
	// find a common edge
	//	
	j = 0;			// 
	
	for (i = 0; i < this->size(); i++)
	{
		p1 = this->points[i].getXYZ();
		p2 = this->points[(i+1) % this->size()].getXYZ();
		for (j = 0; j < f2->size(); j++)
		{
			p3 = f2->points[j].getXYZ();
			p4 = f2->points[(j+1) % f2->size()].getXYZ();
			for (k = 0; k < 3; k++)
			{
				if (fabs(p1[k] - p4[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
				if (fabs(p2[k] - p3[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
			} //end for
			if (k==3)
				break;
		} //end for
		if (j < f2->size())
			break;
	} //end for
	
	if (i == this->size())
		return NULL;			// no matching edges

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = this->points[(i+this->size()-1)%this->size()].getXYZ();
	delta = p1 - back;
	normal.crossProduct (planenormal, delta);
	normal.normalize();

	back = f2->points[(j+2)%f2->size()].getXYZ();
	delta = back - p1;
	dot = delta.dotProduct(normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep1 = (bool)(dot < -CONTINUOUS_EPSILON);
	
	back = this->points[(i+2)%this->size()].getXYZ();
	delta = back - p2;
	normal.crossProduct (planenormal, delta);
	normal.normalize();

	back = f2->points[(j+f2->size()-1)%f2->size()].getXYZ();
	delta = back - p2;
	dot = delta.dotProduct(normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep2 = (bool)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = new edWinding_t ();
	
	// copy first polygon
	for (k=(i+1)%this->size() ; k != i ; k=(k+1)%this->size())
	{
		if (!keep && k==(i+1)%this->size() && !keep2)
			continue;
		
		newf->addPoint(this->points[k]);
	}
	
	// copy second polygon
	for (l= (j+1)%f2->size() ; l != j ; l=(l+1)%f2->size())
	{
		if (!keep && l==(j+1)%f2->size() && !keep1)
			continue;
		newf->addPoint(f2->points[l]);
	}

	return newf;
}
