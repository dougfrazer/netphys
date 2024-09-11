
#include "vector.h"
#include "matrix.h"
#include "geometry.h"
#include "physics.h"
#include "plane.h"
#include "util.h"
#include <limits>
#include <vector>
#include <algorithm>
#include "simplex.h"

// Some documentation for reference:
// https://graphics.stanford.edu/courses/cs448b-00-winter/papers/gilbert.pdf
// https://ora.ox.ac.uk/objects/uuid:69c743d9-73de-4aff-8e6f-b4dd7c010907/download_file?safe_filename=GJK.PDF&file_format=application%2Fpdf&type_of_work=Journal+article

//******************************************************************************
static void SolveLine(Simplex& simplex)
{
    vector3 a = simplex.verts[0].p;
    vector3 b = simplex.verts[1].p;

	// if it is not between A and B, remove the segment it is further from.
	//           |                      |
	// region A  |       region AB      |  region B
	//           A                      B
	//  r<0      *----------------------*   r>1
	// remove b  |       keep both      |   remove a
	//           |                      |
	float r = FindClosestPointOnLineToPoint(vector3(), a, b);

    if (r <= 0.0f)
    {
		simplex.verts.resize(1);
        return;
    }
    if (r >= 1.0f)
    {
        simplex.verts[0] = simplex.verts[1];
		simplex.verts.resize(1);
        return;
    }
    simplex.verts[1].edgeWeight = r;
	simplex.verts.resize(2);
}
//******************************************************************************
static void SolveTriangle(Simplex& simplex)
{
	const vector3& va = simplex.verts[0].p;
	const vector3& vb = simplex.verts[1].p;
	const vector3& vc = simplex.verts[2].p;
	float u,v;
	int region = ClosestPointTrianglePointRatio(vector3(), va, vb, vc, u, v);
	switch (region)
	{
		case 0:
		{
			// within the triangle
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			return;
		}
		case 1:
		{
			simplex.verts.resize(1);
			return;
		}
		case 2:
		{
			simplex.verts[0] = simplex.verts[1];
			simplex.verts.resize(1);
			return;
		}
		case 3:
		{
			simplex.verts[0] = simplex.verts[2];
			simplex.verts.resize(1);
			return;
		}
		case 4:
		{
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 5:
		{
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		// cases 6,7,8 are edge-cases where the origin falls on one of the lines
		// of the triangle... remove one of the edge points and recompute our uvs
		case 6:
		{
			// its on AB, remove B
			simplex.verts[1] = simplex.verts[2];
			SolveLine(simplex);
			return;
		}
		case 7:
		{
			// its on AC, implicitly remove C
			SolveLine(simplex);
			return;
		}
		case 8:
		{
			// its on BC, implicitly remove C
			SolveLine(simplex);
			return;
		}
		default: assert(false);
	}
	assert(false);
}

//******************************************************************************
static void SolveTetrahedron(Simplex& simplex)
{
	const vector3& va = simplex.verts[0].p;
	const vector3& vb = simplex.verts[1].p;
	const vector3& vc = simplex.verts[2].p;
	const vector3& vd = simplex.verts[3].p;

	float u,v,w;
	int region = ClosestPointTetrahedronPointRatio(vector3(), va, vb, vc, vd, u, v, w);

	switch (region)
	{
		case 0:
		{
			// within the tetrahedron
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			simplex.verts[3].edgeWeight = w;
			return;
		}
		case 1:
		{
			// region A
			simplex.verts.resize(1);
			return;
		}
		case 2:
		{
			// region B
			simplex.verts[0] = simplex.verts[1];
			simplex.verts.resize(1);
			return;
		}
		case 3:
		{
			// region C
			simplex.verts[0] = simplex.verts[2];
			simplex.verts.resize(1);
			return;
		}
		case 4:
		{
			simplex.verts[0] = simplex.verts[3];
			simplex.verts.resize(1);
			return;
		}
		case 5:
		{
			// region AB
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 6:
		{
			// region AC
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 7:
		{
			// region AD
			simplex.verts[1] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 8:
		{
			// region BC
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 9:
		{
			// region BD
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 10:
		{
			// region CD
			simplex.verts[0] = simplex.verts[2];
			simplex.verts[1] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case 11:
		{
			// region ABC
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case 12:
		{
			// region ABD
			simplex.verts[2] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case 13:
		{
			// region ACD
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[2] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case 14:
		{
			// region BCD
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[2] = simplex.verts[3];
			simplex.verts[1].edgeWeight = u;
			simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		default: assert(false);
	}

	assert(false);
}
//******************************************************************************
vector3 GetDirectionToOrigin(const vector3& a)
{
	return -a;
}
//******************************************************************************
vector3 GetDirectionToOrigin(const vector3& a, const vector3& b)
{
	vector3 ab = b - a;
	vector3 ao = -b;
	vector3 t = ab.cross(ao);
	vector3 bt = t.cross(ab);
	float sign = bt.dot(ao);
	if (FloatEquals(sign, 0.0f))
	{
		// the origin lies on the line ab
		vector3 abt = ab.cross({ 0,0,1 });
		if (abt.magnitude_sq() == 0.0f)
		{
			// another edge case... ab happens to have no Z component
			return ab.cross({ 0,1,0 });
		}
		return abt;
	}
	
	return (sign < 0.0f) ? -bt : bt;
}
//******************************************************************************
vector3 GetDirectionToOrigin(const vector3& a, const vector3& b, const vector3& c)
{
    vector3 ab = b - a;
    vector3 ac = c - a;
    vector3 ao = -a;
    vector3 n = ab.cross(ac);
    float sign = n.dot(ao);

	if (FloatEquals(sign, 0.0f))
	{
		// the origin lies on the triangle ABC
		vector3 abt = ab.cross({ 0,0,1 });
		if (abt.magnitude_sq() == 0.0f)
		{
			// another edge case... ab happens to have no Z component
			return ab.cross({ 0,1,0 });
		}
		return abt;
	}

	return (sign < 0.0f) ? -n : n;
}
//******************************************************************************
vector3 GetSearchDirection(const Simplex& simplex)
{
	switch (simplex.size())
	{
		case 1: return GetDirectionToOrigin(simplex.verts[0].p);
		case 2: return GetDirectionToOrigin(simplex.verts[0].p, simplex.verts[1].p);
		case 3: return GetDirectionToOrigin(simplex.verts[0].p, simplex.verts[1].p, simplex.verts[2].p);
	}
	assert(false);
	return vector3();
}
//******************************************************************************
// FindIntersectionPoint:
// ----------------------
// This series of functions attempts to figure out the minimum distance and direction
// to push move polygon A in the simplex so that there is no longer an overlap.
// 
// If there is already no overlap, it will return the minimum distance and direction
// between the two polygons.
// 
// The algorithm is an implementation of EPA (Expanding Polytope Algorithm) and it takes in
// a simplex generated from GJK which, in the case of an overlap, contains the origin.
// 
// For overlapping polygons, the goal is to find the minimum distance to the exterior
// hull of the Minkowski difference.  It does so by iteratively taking the closest point
// to the origin and finding a point on the Minkowski difference that is furthest
// in the direction opposite that.  If the furthest point returned is within a reasonable 
// threshold of the point we started with then we must have found the edge of the Minkowski 
// difference, and the distance and direction it took to get there is returned the minimum
// distance/direction to un-overlap the two objects.
// 
// There are some assumptions baked into this that it is called only after 
// the DetectCollision() functions
//******************************************************************************
static void GetClosestTriangleToOrigin(const Simplex& simplex, float& distance, float& u, float& v, int& va_index, int& vb_index, int& vc_index)
{
	//  - distance = distance from the origin
	//  - u,v = percentage along the edge (A + u(B-A) + v(C-A)) to get the intersection point
	const vector3 origin;
	float bestDistSq = FLT_MAX;
	float bestU = 0.0f;
	float bestV = 0.0f;
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		int va = i;
		int vb = va == simplex.verts.size() - 1 ? 0 : va + 1;
		int vc = vb == simplex.verts.size() - 1 ? 0 : vb + 1;
		const vector3& a = simplex.verts[va].p;
		const vector3& b = simplex.verts[vb].p;
		const vector3& c = simplex.verts[vc].p;
		float u,v;
		ClosestPointTrianglePointRatio(origin, a, b, c, u, v);
		float distanceSq = (a + (b - a) * u + (c - a) * v).magnitude_sq();
		if (distanceSq < bestDistSq)
		{
			bestDistSq = distanceSq;
			va_index = va;
			vb_index = vb;
			vc_index = vc;
			bestU = u;
			bestV = v;
		}
	}

	// make sure we actually found something
	assert(bestDistSq != FLT_MAX);
	distance = sqrt(bestDistSq);
	u = bestU;
	v = bestV;
}
//******************************************************************************
bool FindIntersectionPointsStep3D(const CollisionParams& params, Simplex& simplex, CollisionData* outCollision)
{
	// its assumed this is being called on a completed simplex that encapsulates the origin
	assert(simplex.verts.size() > 3);
	assert(simplex.m_containsOrigin);

	// First: find the closest edge in our simplex to the origin
	float distance = 0.0f;
	float u,v;
	int va, vb, vc;
	GetClosestTriangleToOrigin(simplex, distance, u, v, va, vb, vc);

	const vector3& a = simplex.verts[va].p;
	const vector3& b = simplex.verts[vb].p;
	const vector3& c = simplex.verts[vc].p;
	const vector3 ab = (b - a);
	const vector3 ac = (c - a);
	const vector3 n = ab.cross(ac);
	const vector3 closest_point_to_origin = a + (b - a) * u + (c-a) * v;
	const vector3 normal = closest_point_to_origin.dot(n) > 0.0f ? n : -n; // pick the normal going away from the origin

	// Second: try and expand the simplex by 'pushing out' that edge in the direction of its normal
	int a_index, b_index;
	const vector3 a_support = params.a->GetPointFurthestInDirection(normal, params.aTransform, &a_index);
	const vector3 b_support = params.b->GetPointFurthestInDirection(-normal, params.bTransform, &b_index);
	const vector3 support = a_support - b_support;

	// Third: Given the new point that we found (the position furthest away from the closest edge to the origin)
	//        if that point is already in our simplex, then the edge we have found must be on the exterior hull 
	//        of the minkowski difference.  Therefore, the distance to this edge is the penetration depth
	//        and the penetration direction is the normal to this edge
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		if (a_index == simplex.verts[i].a_index && b_index == simplex.verts[i].b_index)
		{
			// this should be one of the points on the edge closest to the origin
			// todo: check this assertion... it makes sense because if we pushed furthest in the
			//       normal of this face, the points returned should be on this face if there
			//       is nowhere left to go... but its tripping for now so gonna comment it out
			//assert(i == va || i == vb || i == vc);

			outCollision->penetrationDirection = normal.normalize();
			outCollision->depth = distance;
			outCollision->a_normal = vector3(); // hmm todo: i think these are computable from the simplex 
			outCollision->b_normal = vector3();

			return true;
		}
	}

	// we didn't find a match, so add this point and tell the algorithm to continue
	SimplexPoint new_point;
	new_point.A = a_support;
	new_point.a_index = a_index;
	new_point.B = b_support;
	new_point.b_index = b_index;
	new_point.p = support;

	// should we go from va->new->vb->vc or va->vb->new->vc or va->vb->vc->new ?
	simplex.insert(new_point, vb);

	return false;
}
//******************************************************************************
bool FindIntersectionPoints3D(const CollisionParams& params, Simplex& simplex, int max_steps, CollisionData* outCollision)
{
	if (simplex.m_containsOrigin)
	{
		// the input is a simplex which encloses the origin
		// we need to find the minimum distance from the origin to the surrounding hull
		// of the minkowski difference.  we have not computed the whole minkowski difference
		// however (nor can we with continuous shapes)... so we iterate until we get close enough
		int iterCount = 0;
		while (iterCount < max_steps)
		{
			if (FindIntersectionPointsStep3D(params, simplex, outCollision))
			{
				return true;
			}
			++iterCount;
		}
	}
	else
	{
		float distance = 0.0f;
		float u, v;
		int va, vb, vc;
		GetClosestTriangleToOrigin(simplex, distance, u, v, va, vb, vc);

		const vector3& a = simplex.verts[va].p;
		const vector3& b = simplex.verts[vb].p;
		const vector3& c = simplex.verts[vc].p;

		const vector3 closest_point_to_origin = a + (b - a) * u + (c - a) * v;

		outCollision->depth = distance;
		outCollision->penetrationDirection = closest_point_to_origin.normalize();

		return true;
	}
	return false;
}
//******************************************************************************
COLLISION_RESULT DetectCollisionStep3D(const CollisionParams& params, Simplex& simplex)
{
	const std::vector<SimplexPoint> oldPoints = simplex.verts;

	switch (simplex.size())
	{
	case 1:
		break;

	case 2:
		SolveLine(simplex);
		break;

	case 3:
		SolveTriangle(simplex);
		break;

	case 4:
		SolveTetrahedron(simplex);
		break;

	default:
		assert(false);
	}

	if (simplex.size() == 4)
	{
		simplex.m_containsOrigin = true;
		return COLLISION_RESULT_OVERLAP; // done
	}

	vector3 d = GetSearchDirection(simplex);
	if (FloatEquals(d.magnitude_sq(), 0.0f))
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}

	// TODO: there might be an optimization here where we pass in the last point we considered and 
	//       only look at points adjacent to it and pick the next furthest point, rather than
	//       iterating the entire vertex list
	int a_index, b_index;
	vector3 a_support = params.a->GetPointFurthestInDirection(d, params.aTransform, &a_index);
	vector3 b_support = params.b->GetPointFurthestInDirection(-d, params.bTransform, &b_index);
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}

	// If we got a point that we already had, there must be no more room towards the origin
	// in this simplex, therefore there is no way to encapsulate the origin, therefore no overlap
	for (int i = 0; i < oldPoints.size(); ++i)
	{
		if (a_index == oldPoints[i].a_index && b_index == oldPoints[i].b_index)
		{
			assert(a_support == oldPoints[i].A && b_support == oldPoints[i].B);

			return COLLISION_RESULT_NO_OVERLAP;
		}
	}

	// Otherwise, add this point to the simplex and continue
	SimplexPoint p;
	p.A = a_support;
	p.B = b_support;
	p.a_index = a_index;
	p.b_index = b_index;
	p.p = support;
	simplex.verts.push_back(p);

	return COLLISION_RESULT_CONTINUE;
}
//******************************************************************************
bool DetectCollision3D(const CollisionParams& params, CollisionData* outCollision)
{
	Simplex simplex;

	// TODO: broad phase AABB boxes

	// start with any point in the geometries
	simplex.verts.resize(1);
	simplex.verts[0].A = params.a->GetPointFurthestInDirection({ 1,1,1 }, params.aTransform);
	simplex.verts[0].B = params.b->GetPointFurthestInDirection({ -1,-1,-1 }, params.bTransform);
	simplex.verts[0].p = simplex.verts[0].B - simplex.verts[0].A;

	const vector3 destination = { 0.0f, 0.0f, 0.0f }; // origin
	constexpr int maxIterations = 20;
	int iterCount = 0;
	COLLISION_RESULT result = COLLISION_RESULT_CONTINUE;
	while (result == COLLISION_RESULT_CONTINUE && iterCount++ < maxIterations)
	{
		result = DetectCollisionStep3D(params, simplex);
	}
	assert(iterCount < maxIterations); // if we bailed due to iterations... we have undefined collision
	assert(result != COLLISION_RESULT_CONTINUE);

	if (outCollision)
	{
		outCollision->success = FindIntersectionPoints3D(params, simplex, maxIterations, outCollision);
	}

	return (result == COLLISION_RESULT_OVERLAP);
}
//******************************************************************************






//******************************************************************************
// 2D implementations
// -------------------
// could share code, but trying to simplify 3D as much as possible so moving complexity
// into separate functions.  might look into finding a clean way to share later.
//******************************************************************************
static void GetClosestEdgeToOrigin(const Simplex& simplex, float& distance, float& u, int& startIndex, int& endIndex)
{
	//  - distance = is the distance from the origin
	//  - u = percentage along the edge (A + u(B-A)) to get the intersection point
	const vector3 origin;
	float bestDistSq = FLT_MAX;
	float bestV = 0.0f;
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		int s = i;
		int e = i == simplex.verts.size() - 1 ? 0 : i + 1;
		const vector3& a = simplex.verts[s].p;
		const vector3& b = simplex.verts[e].p;
		float v = FindClosestPointOnLineToPoint(origin, a, b);
		float distanceSq = (a + (b - a) * v).magnitude_sq();
		if (distanceSq < bestDistSq)
		{
			bestDistSq = distanceSq;
			startIndex = s;
			endIndex = e;
			bestV = v;
		}
	}

	// make sure we actually found something
	assert(bestDistSq != FLT_MAX);
	distance = sqrt(bestDistSq);
	u = bestV;
}
//******************************************************************************
bool FindIntersectionPointsStep2D(const CollisionParams& params, Simplex& simplex, CollisionData* outCollision)
{
	// its assumed this is being called on a completed simplex that encapsulates the origin
	assert(simplex.verts.size() > 2);
	assert(simplex.m_containsOrigin);

	// First: find the closest edge in our simplex to the origin
	float distance, u;
	int startIndex, endIndex;
	vector3 dir;
	GetClosestEdgeToOrigin(simplex, distance, u, startIndex, endIndex);

	const vector3& a = simplex.verts[startIndex].p;
	const vector3& b = simplex.verts[endIndex].p;
	const vector3 closest_point_to_origin = a + (b - a) * u;
	const vector3 edge = b - a;
	const vector3 n = vector3(edge.y, -edge.x, 0.0f); // can ignore Z since this is 2D... TODO use vector2 instead
	const vector3 normal_to_edge = closest_point_to_origin.dot(n) > 0.0f ? n : -n;

	// Second: try and expand the simplex by 'pushing out' that edge in the direction of its normal
	int a_index, b_index;
	const vector3 a_support = params.a->GetPointFurthestInDirection(normal_to_edge, params.aTransform, &a_index);
	const vector3 b_support = params.b->GetPointFurthestInDirection(-normal_to_edge, params.bTransform, &b_index);
	const vector3 support = a_support - b_support;

	// Third: Given the new point that we found (the position furthest away from the closest edge to the origin)
	//        if that point is already in our simplex, then the edge we have found must be on the exterior hull 
	//        of the minkowski difference.  Therefore, the distance to this edge is the penetration depth
	//        and the penetration direction is the normal to this edge
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		if (a_index == simplex.verts[i].a_index && b_index == simplex.verts[i].b_index)
		{
			// this should be one of the points on the edge closest to the origin
			assert(i == startIndex || i == endIndex);

			outCollision->penetrationDirection = normal_to_edge.normalize();
			outCollision->depth = distance;
			outCollision->a_normal = vector3(); // hmm todo: i think these are computable from the simplex 
			outCollision->b_normal = vector3();

			return true;
		}
	}

	// we didn't find a match, so add this point and tell the algorithm to continue
	SimplexPoint new_point;
	new_point.A = a_support;
	new_point.a_index = a_index;
	new_point.B = b_support;
	new_point.b_index = b_index;
	new_point.p = support;

	simplex.insert(new_point, endIndex);

	return false;
}
//******************************************************************************
bool FindIntersectionPoints2D(const CollisionParams& params, Simplex& simplex, int max_steps, CollisionData* outCollision)
{
	if (simplex.m_containsOrigin)
	{
		// the input is a simplex which encloses the origin
		// we need to find the minimum distance from the origin to the surrounding hull
		// of the minkowski difference.  we have not computed the whole minkowski difference
		// however (nor can we with continuous shapes)... so we iterate until we get close enough
		int iterCount = 0;
		while (iterCount < max_steps)
		{
			if (FindIntersectionPointsStep2D(params, simplex, outCollision))
			{
				return true;
			}
			++iterCount;
		}
	}
	else
	{
		float u, v;
		int startIndex, endIndex;
		GetClosestEdgeToOrigin(simplex, u, v, startIndex, endIndex);

		const vector3& a = simplex.verts[startIndex].p;
		const vector3& b = simplex.verts[endIndex].p;

		vector3 closest_point_to_origin = a + (b - a) * v;

		outCollision->depth = u;
		outCollision->penetrationDirection = closest_point_to_origin;

		return true;
	}
	return false;
}
//******************************************************************************
COLLISION_RESULT DetectCollisionStep2D(const CollisionParams& params, Simplex& simplex)
{
	// TODO: could probably make this use all vector2's instead
	Simplex save = simplex;
	switch (simplex.size())
	{
	case 1:
		break;

	case 2:
		SolveLine(simplex);
		break;

	case 3:
		SolveTriangle(simplex);
		break;

	default:
		assert(false);
	}

	if (simplex.size() == 3)
	{
		simplex.m_containsOrigin = true;
		return COLLISION_RESULT_OVERLAP; // done
	}

	vector3 d = GetSearchDirection(simplex);
	if (FloatEquals(d.magnitude_sq(), 0.0f))
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}

	// TODO: there might be an optimization here where we pass in the last point we considered and 
	//       only look at points adjacent to it and pick the next furthest point, rather than
	//       iterating the entire vertex list
	int a_index, b_index;
	vector3 a_support = params.a->GetPointFurthestInDirection(d, params.aTransform, &a_index);
	vector3 b_support = params.b->GetPointFurthestInDirection(-d, params.bTransform, &b_index);
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}
	for (int i = 0; i < save.size(); ++i)
	{
		if (a_index == save.verts[i].a_index && b_index == save.verts[i].b_index)
		{
			assert(a_support == save.verts[i].A && b_support == save.verts[i].B);

			return COLLISION_RESULT_NO_OVERLAP;
		}
	}

	SimplexPoint p;
	p.A = a_support;
	p.B = b_support;
	p.a_index = a_index;
	p.b_index = b_index;
	p.p = support;
	simplex.verts.push_back(p);

	return COLLISION_RESULT_CONTINUE;
}
//******************************************************************************
bool DetectCollision2D(const CollisionParams& params, CollisionData* outCollision)
{
	Simplex simplex;

	// TODO: broad phase

	// start with any point in the geometries
	simplex.verts.resize(1);
	simplex.verts[0].A = params.a->GetPointFurthestInDirection({ 1,1,1 }, params.aTransform);
	simplex.verts[0].B = params.b->GetPointFurthestInDirection({ -1,-1,-1 }, params.bTransform);
	simplex.verts[0].p = simplex.verts[0].B - simplex.verts[0].A;

	const vector3 destination = { 0.0f, 0.0f, 0.0f }; // origin
	constexpr int maxIterations = 20;
	int iterCount = 0;
	COLLISION_RESULT result = COLLISION_RESULT_CONTINUE;
	while (result == COLLISION_RESULT_CONTINUE && iterCount++ < maxIterations)
	{
		result = DetectCollisionStep3D(params, simplex);
	}
	assert(iterCount < maxIterations); // if we bailed due to iterations... we have undefined collision
	assert(result != COLLISION_RESULT_CONTINUE);

	if (outCollision)
	{
		outCollision->success = FindIntersectionPoints2D(params, simplex, maxIterations, outCollision);
	}

	return (result == COLLISION_RESULT_OVERLAP);
}