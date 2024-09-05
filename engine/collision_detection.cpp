
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
	else if (sign < 0.0f)
	{
		return -bt;
	}

	return bt;
}
//******************************************************************************
vector3 GetDirectionToOrigin(const vector3& a, const vector3& b, const vector3& c)
{
    vector3 ab = b - a;
    vector3 ac = c - a;
    vector3 ao = -a;
    vector3 n = ab.cross(ac);
    float sign = n.dot(ao);
	assert(!FloatEquals(sign, 0.0f)); // todo: implement this case... the origin lies on the triangle.
	if (sign < 0.0f)
	{
		return -n;
	}

	return -n;
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
COLLISION_RESULT DetectCollisionStep(const CollisionParams& params, Simplex& simplex)
{
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

		case 4:
			SolveTetrahedron(simplex);
			break;

		default:
			assert(false);
	}

	if (simplex.size() == 4 || (!params.solve3D && simplex.size() == 3))
	{
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
void GetClosestEdgeToOrigin(const Simplex& simplex, float& u, float& v, int& startIndex, int& endIndex)
{
	// its assumed this is being called on a completed simplex that encapsulates the origin

	// similar to GetFurthestEdgeInDirection_Collision()
	//  - u is the distance from the origin
	//  - v is the percentage along the edge (A + v(B-A)) to get the intersection point
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
	u = sqrt(bestDistSq);
	v = bestV;
}
//******************************************************************************
// Take a simplex which necessarily contains the origin and push its edges out until
// you get to an exterior hull edge within a certain tolerance, then return that point
// as the closest point
//******************************************************************************
bool FindIntersectionPointsStep_Collision(const CollisionParams& params, Simplex& simplex, float& outDepth, vector3& outPenetrationVector)
{
	float u, v;
	int startIndex, endIndex;
	vector3 dir;
	assert(simplex.verts.size() > 2);
	GetClosestEdgeToOrigin(simplex,u,v,startIndex,endIndex);

	const vector3& a = simplex.verts[startIndex].p;
	const vector3& b = simplex.verts[endIndex].p;

	vector3 closest_point_to_origin = a + (b-a)*v;
	vector3 edge = b - a;
	vector3 normal_to_edge = vector3(edge.y, -edge.x, closest_point_to_origin.z); // not sure this is right, i think we need to do the normal to the face but not sure what third point should be

	if (closest_point_to_origin.dot(normal_to_edge) < 0.0f)
	{
		normal_to_edge = -normal_to_edge;
	}

	int a_index, b_index;
	vector3 a_support = params.a->GetPointFurthestInDirection(normal_to_edge, params.aTransform, &a_index);
	vector3 b_support = params.b->GetPointFurthestInDirection(-normal_to_edge, params.bTransform, &b_index);
	vector3 support = a_support - b_support;

	for (int i = 0; i < simplex.verts.size(); i++)
	{
		if (a_index == simplex.verts[i].a_index && b_index == simplex.verts[i].b_index)
		{
			// we already have this point in our simplex
			// it should be one of the two points on the edge we computed
			// therefore this edge is an exterior edge of the minkowski difference, and we can return
			// the intersection point in the direction as the furthest point
			assert(i == startIndex || i == endIndex);

			outPenetrationVector = normal_to_edge.normalize();
			outDepth = u;

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
bool FindIntersectionPoints(const CollisionParams& params, Simplex& simplex, bool collision, int max_steps, float& depth, vector3& penetrationVector)
{
	if (collision)
	{
		// the input is a simplex which encloses the origin
		// we need to find the minimum distance from the origin to the surrounding hull
		// of the minkowski difference.  we have not computed the whole minkowski difference
		// however (nor can we with continuous shapes)... so we iterate until we get close enough
		int iterCount = 0;
		while (iterCount < max_steps)
		{
			if (FindIntersectionPointsStep_Collision(params, simplex, depth, penetrationVector))
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
		depth = u;
		penetrationVector = closest_point_to_origin;

		return true;
	}
	return false;
}

//******************************************************************************
bool DetectCollision(const CollisionParams& params, CollisionData* outCollision)
{
    Simplex simplex;

    // TODO: broad phase

    // start with any point in the geometries
	simplex.verts.resize(1);
	simplex.verts[0].A = params.a->GetPointFurthestInDirection({1,1,1}, params.aTransform);
	simplex.verts[0].B = params.b->GetPointFurthestInDirection({-1,-1,-1}, params.bTransform);
    simplex.verts[0].p = simplex.verts[0].B - simplex.verts[0].A;

    const vector3& destination = {0.0f, 0.0f, 0.0f}; // origin
    constexpr int maxIterations = 20;
    int iterCount = 0;
    COLLISION_RESULT result = COLLISION_RESULT_CONTINUE;
    while (result == COLLISION_RESULT_CONTINUE && iterCount++ < maxIterations)
    {
        result = DetectCollisionStep(params, simplex);
    }
    assert(iterCount < maxIterations); // if we bailed due to iterations... we have undefined collision
    assert(result != COLLISION_RESULT_CONTINUE);
	const bool overlap = result == COLLISION_RESULT_OVERLAP;

	if (outCollision)
	{
		vector3 penetrationDirection;
		float depth;
		outCollision->success = FindIntersectionPoints(params, simplex, overlap, 20, depth, penetrationDirection);
		outCollision->penetrationDirection = penetrationDirection;
		outCollision->depth = depth;
	}

    return (result == COLLISION_RESULT_OVERLAP);
}