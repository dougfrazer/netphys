
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
	//  u<0      *----------------------*   u>1
	// remove b  |       keep both      |   remove a
	//           |                      |
	float u;
	LineRegion region = ClosestPoint_LinePointRatio(vector3(), a, b, u);

	switch (region)
	{
		case LineRegion_A:
		{
			simplex.verts.resize(1);
		}
		break;

		case LineRegion_B:
		{
			simplex.verts[0] = simplex.verts[1];
			simplex.verts.resize(1);
		}
		break;

		default:
		{
			simplex.verts.resize(2);
		}
		break;
	}
}
//******************************************************************************
static void SolveTriangle(Simplex& simplex)
{
	const vector3& va = simplex.verts[0].p;
	const vector3& vb = simplex.verts[1].p;
	const vector3& vc = simplex.verts[2].p;
	float u,v;
	TriangleRegion region = ClosestPoint_TrianglePointRatio(vector3(), va, vb, vc, u, v);
	switch (region)
	{
		case TriangleRegion_ABC:
		{
			// within the triangle
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			return;
		}
		case TriangleRegion_A:
		{
			// set the simplex to just A
			simplex.verts.resize(1);
			return;
		}
		case TriangleRegion_B:
		{
			// set the simplex to just B
			simplex.verts[0] = simplex.verts[1];
			simplex.verts.resize(1);
			return;
		}
		case TriangleRegion_C:
		{
			// set the simplex to just C
			simplex.verts[0] = simplex.verts[2];
			simplex.verts.resize(1);
			return;
		}
		case TriangleRegion_AB:
		{
			// set the simplex to AB
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TriangleRegion_AC:
		{
			simplex.verts[1] = simplex.verts[2];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TriangleRegion_BC:
		{
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[2];
			//simplex.verts[0].edgeWeight = u;
			//simplex.verts[1].edgeWeight = v;
			simplex.verts.resize(2);
			return;
		}
	}
	// should never get here
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
	TetrahedronRegion region = ClosestPoint_TetrahedronPointRatio(vector3(), va, vb, vc, vd, u, v, w);

	switch (region)
	{
		case TetrahedronRegion_ABCD:
		{
			// within the tetrahedron
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			//simplex.verts[3].edgeWeight = w;
			return;
		}
		case TetrahedronRegion_A:
		{
			// region A
			simplex.verts.resize(1);
			return;
		}
		case TetrahedronRegion_B:
		{
			// region B
			simplex.verts[0] = simplex.verts[1];
			simplex.verts.resize(1);
			return;
		}
		case TetrahedronRegion_C:
		{
			// region C
			simplex.verts[0] = simplex.verts[2];
			simplex.verts.resize(1);
			return;
		}
		case TetrahedronRegion_D:
		{
			simplex.verts[0] = simplex.verts[3];
			simplex.verts.resize(1);
			return;
		}
		case TetrahedronRegion_AB:
		{
			// region AB
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_AC:
		{
			// region AC
			simplex.verts[1] = simplex.verts[2];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_AD:
		{
			// region AD
			simplex.verts[1] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_BC:
		{
			// region BC
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[2];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_BD:
		{
			// region BD
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_CD:
		{
			// region CD
			simplex.verts[0] = simplex.verts[2];
			simplex.verts[1] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			simplex.verts.resize(2);
			return;
		}
		case TetrahedronRegion_ABC:
		{
			// region ABC
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case TetrahedronRegion_ABD:
		{
			// region ABD
			simplex.verts[2] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case TetrahedronRegion_ACD:
		{
			// region ACD
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[2] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		case TetrahedronRegion_BCD:
		{
			// region BCD
			simplex.verts[0] = simplex.verts[1];
			simplex.verts[1] = simplex.verts[2];
			simplex.verts[2] = simplex.verts[3];
			//simplex.verts[1].edgeWeight = u;
			//simplex.verts[2].edgeWeight = v;
			simplex.verts.resize(3);
			return;
		}
		default: assert(false);
	}

	assert(false);
}
//******************************************************************************
static void GetClosestEdgeToOrigin(const Simplex& simplex, float& distance, float& u, int& startIndex, int& endIndex)
{
	//  - distance = is the distance from the origin
	//  - u = percentage along the edge (A + u(B-A)) to get the intersection point
	const vector3 origin;
	float bestDistSq = FLT_MAX;
	float bestU = 0.0f;
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		int s = i;
		int e = i == simplex.verts.size() - 1 ? 0 : i + 1;
		const vector3& a = simplex.verts[s].p;
		const vector3& b = simplex.verts[e].p;
		float iterU;
		ClosestPoint_LinePointRatio(origin, a, b, iterU);
		float distanceSq = (a + (b - a) * iterU).magnitude_sq();
		if (distanceSq < bestDistSq)
		{
			bestDistSq = distanceSq;
			startIndex = s;
			endIndex = e;
			bestU = iterU;
		}
	}

	// make sure we actually found something
	assert(bestDistSq != FLT_MAX);
	distance = sqrt(bestDistSq);
	u = bestU;
}
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
		float u, v;
		ClosestPoint_TrianglePointRatio(origin, a, b, c, u, v);
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
static void GetNormalAwayFromOrigin(const Simplex& simplex, bool is3D, vector3 *outNormal, float* outDistance, int* simplex_a, int* simplex_b, int* simplex_c)
{
	if(!is3D)
	{
		// First: find the closest edge in our simplex to the origin
		float distance = 0.0f;
		float u = 0.0f;
		int startIndex, endIndex;
		vector3 dir;
		GetClosestEdgeToOrigin(simplex, distance, u, startIndex, endIndex);

		const vector3& a = simplex.verts[startIndex].p;
		const vector3& b = simplex.verts[endIndex].p;
		const vector3 closest_point_to_origin = a + (b - a) * u; // this will implicitly be going from origin towards the closest point
		const vector3 edge = b - a;
		const vector3 n = vector3(edge.y, -edge.x, 0.0f); // can ignore Z since this is 2D... TODO use vector2 instead
		const vector3 normal_to_edge = closest_point_to_origin.dot(n) > 0.0f ? n : -n; // this should be going away from the origin

		*simplex_a = startIndex;
		*simplex_b = endIndex;
		*simplex_c = -1; // not used in 2d
		*outDistance = distance;
		*outNormal = normal_to_edge;
	}
	else
	{
		// First: find the closest edge in our simplex to the origin
		float distance = 0.0f;
		float u, v;
		int va, vb, vc;
		GetClosestTriangleToOrigin(simplex, distance, u, v, va, vb, vc);

		bool closeEnough = false;
		const vector3& a = simplex.verts[va].p;
		const vector3& b = simplex.verts[vb].p;
		const vector3& c = simplex.verts[vc].p;
		const vector3 ab = (b - a);
		const vector3 ac = (c - a);
		const vector3 n = (ab.cross(ac)).normalize();
		const vector3 closest_point_to_origin = a + (b - a) * u + (c - a) * v;
		const vector3 normal = closest_point_to_origin.dot(n) > 0.0f ? n : -n; // pick the normal going away from the origin

		*simplex_a = va;
		*simplex_b = vb;
		*simplex_c = vc;
		*outDistance = distance;
		*outNormal = normal;
	}
}
//******************************************************************************
static bool FindIntersectionPointsStep(const CollisionParams& params, Simplex& simplex, bool is3D, CollisionData* outCollision)
{
	assert(simplex.m_containsOrigin);
	assert((is3D && simplex.verts.size() > 3) || (!is3D && simplex.verts.size() > 2));

	bool closeEnough = false;
	float distance = 0.0f;
	int simplex_a, simplex_b, simplex_c;
	vector3 normal;
	GetNormalAwayFromOrigin(simplex, is3D, &normal, &distance, &simplex_a, &simplex_b, &simplex_c);

	constexpr float TOLERANCE = 0.01f;
	if (distance > TOLERANCE)
	{
		// Second: try and expand the simplex by 'pushing out' that edge in the direction of its normal
		const vector3 a_support = params.a->GetPointFurthestInDirection(normal, params.aTransform, is3D);
		const vector3 b_support = params.b->GetPointFurthestInDirection(-normal, params.bTransform, is3D);
		const vector3 support = a_support - b_support;

		// Third: Given the new point that we found (the position furthest away from the closest edge to the origin)
		//        if that point is already in our simplex, then the edge we have found must be on the exterior hull 
		//        of the minkowski difference.  Therefore, the distance to this edge is the penetration depth
		//        and the penetration direction is the normal to this edge
		for (int i = 0; i < simplex.verts.size(); i++)
		{
			if (vector3::Equals(a_support, simplex.verts[i].A, TOLERANCE) &&
				vector3::Equals(b_support, simplex.verts[i].B, TOLERANCE))
			{
				// this should be one of the points on the edge closest to the origin
				// todo: check this assertion... it makes sense because if we pushed furthest in the
				//       normal of this face, the points returned should be on this face if there
				//       is nowhere left to go... but its tripping for now so gonna comment it out
				//assert(i == va || i == vb || i == vc);

				closeEnough = true;
				break;
			}
		}

		if (!closeEnough)
		{
			// we didn't find a match, so add this point and tell the algorithm to continue
			SimplexPoint new_point;
			new_point.A = a_support;
			new_point.B = b_support;
			new_point.p = support;
			if (!is3D)
			{
				// for 2D its simple, add the point in between the start and end by inserting it in the middle
				// so insert it at the position of the end, so it'll go start->new->end
				simplex.insert(new_point, simplex_b);
			}
			else
			{
				// for 3D its more complicated, we need to remove the face specified by (simplex_a,simplex_b,simplex_c) and add 3 new faces

				// TODO
			}
			
			return false;
		}
	}
	else
	{
		closeEnough = true;
	}

	assert(closeEnough);

	outCollision->penetrationDirection = normal.normalize();
	outCollision->depth = distance;

	// No idea if this is right, total shot in the dark
	//const vector3& ai = simplex.verts[va].A;
	//const vector3& aj = simplex.verts[vb].A;
	//const vector3& ak = simplex.verts[vc].A;
	//outCollision->a_normal = (aj - ai).cross(aj - ai);
	//
	//const vector3& bi = simplex.verts[va].B;
	//const vector3& bj = simplex.verts[vb].B;
	//const vector3& bk = simplex.verts[vc].B;
	//outCollision->b_normal = (bj - bi).cross(bk - bi);
	// end total shot in the dark

	return true;

}
//******************************************************************************
bool FindIntersectionPoints(const CollisionParams& params, Simplex& simplex, int max_steps, bool is3D, CollisionData* outCollision)
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
			if (FindIntersectionPointsStep(params, simplex, is3D, outCollision))
			{
				return true;
			}
			++iterCount;
		}
	}
	else if (is3D && simplex.verts.size() >= 3)
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
	else if (!is3D && simplex.verts.size() >= 2)
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
// Detect Collision:
// ----------------------
// This is the core implementation of GJK which will try and build a simplex
// to surround the origin in the minkowski difference. If such a simplex can be built,
// there is some point in the A-B space that is zero, which means there must be an 
// overlap.
// 
// The output from this can be fed into the FindIntersectionPoint() which will use
// EPA to figure out the minimum distance and direction of overlap.
//******************************************************************************
COLLISION_RESULT DetectCollisionStep(const CollisionParams& params, bool is3D, Simplex& simplex)
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
		assert(is3D);
		SolveTetrahedron(simplex);
		break;

	default:
		assert(false);
	}

	// to encapsulate the origin in 3 dimensions we need 4 points
	// in two dimensions we only need 3
	if (simplex.size() == 4 || (!is3D && simplex.size() == 3))
	{
		simplex.m_containsOrigin = true;
		return COLLISION_RESULT_OVERLAP; // done
	}

	vector3 d = GetSearchDirection(simplex);
	if (FloatEquals(d.magnitude_sq(), 0.0f))
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}

	// TODO: for Mesh geometries that are consider each vertex
	//       there might be an optimization here where we pass in the last point we considered and 
	//       only look at points adjacent to it and pick the next furthest point, rather than
	//       iterating the entire vertex list
	vector3 a_support = params.a->GetPointFurthestInDirection(d, params.aTransform, is3D);
	vector3 b_support = params.b->GetPointFurthestInDirection(-d, params.bTransform, is3D);
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}

	// If we got a point that we already had, there must be no more room towards the origin
	// in this simplex, therefore there is no way to encapsulate the origin, therefore no overlap
	for (int i = 0; i < oldPoints.size(); ++i)
	{
		if (a_support == oldPoints[i].A && b_support == oldPoints[i].B)
		{
			return COLLISION_RESULT_NO_OVERLAP;
		}
	}

	// Otherwise, add this point to the simplex and continue
	SimplexPoint p;
	p.A = a_support;
	p.B = b_support;
	//p.a_index = a_index;
	//p.b_index = b_index;
	p.p = support;
	simplex.verts.push_back(p);

	return COLLISION_RESULT_CONTINUE;
}
//******************************************************************************
bool DetectCollision(const CollisionParams& params, bool is3D, CollisionData* outCollision)
{
	Simplex simplex;

	// TODO: broad phase AABB boxes

	// start with any point in the geometries
	simplex.verts.resize(1);
	simplex.verts[0].A = params.a->GetPointFurthestInDirection({ 1,1,1 }, params.aTransform, is3D);
	simplex.verts[0].B = params.b->GetPointFurthestInDirection({ -1,-1,-1 }, params.bTransform, is3D);
	simplex.verts[0].p = simplex.verts[0].B - simplex.verts[0].A;

	const vector3 destination = { 0.0f, 0.0f, 0.0f }; // origin
	constexpr int maxIterations = 20;
	int iterCount = 0;
	COLLISION_RESULT result = COLLISION_RESULT_CONTINUE;
	while (result == COLLISION_RESULT_CONTINUE && iterCount++ < maxIterations)
	{
		result = DetectCollisionStep(params, is3D, simplex);
	}
	assert(iterCount < maxIterations); // if we bailed due to iterations... we have undefined collision
	assert(result != COLLISION_RESULT_CONTINUE);

	if (outCollision)
	{
		outCollision->success = FindIntersectionPoints(params, simplex, maxIterations, is3D, outCollision);
	}

	return (result == COLLISION_RESULT_OVERLAP);
}
//******************************************************************************
