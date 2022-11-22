
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
	float r = ClosestPointLinePointRatio(vector3(), a, b);

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
	assert(!FloatEquals(sign, 0.0f)); // todo: implement this case... the origin lies on the triangle
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

	vector3 a_support = params.a->Support(d, params.aTransform);
	vector3 b_support = params.b->Support(-d, params.bTransform);
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_RESULT_NO_OVERLAP;
	}
	for (int i = 0; i < save.size(); ++i)
	{
		if (a_support == save.verts[i].A && b_support == save.verts[i].B)
		{
			return COLLISION_RESULT_NO_OVERLAP;
		}
	}
	SimplexPoint p;
	p.A = a_support;
	p.B = b_support;
	p.p = support;
	simplex.verts.push_back(p);
    return COLLISION_RESULT_CONTINUE;
}

//******************************************************************************
bool FindCollisionDepthStep(const CollisionParams& params, Simplex& simplex, float& outDepth, vector3& outA, vector3& outB, vector3& outp)
{
	const vector3 origin;
	outDepth = FLT_MAX;
	int shortestIndex[2];
	//float bestT2 = 0.0f;
	float bestU = FLT_MAX;
	float bestV = 0.0f;
	for (int i = 0; i < simplex.verts.size(); i++)
	{
		int startIndex = i;
		int endIndex = i == simplex.verts.size() - 1 ? 0 : i + 1;
		const vector3& a = simplex.verts[startIndex].p;
		const vector3& b = simplex.verts[endIndex].p;

		vector3 ab = b - a;
		// the amount of scalar to apply to the direction to intersect with the line ab
		vector3 t = params.aDir.cross(ab);
		if(t.IsNone())
			continue;

		float u = a.cross(ab).magnitude() / t.magnitude();
		if (u < 0.0f)
			continue;

		if (u < bestU)
		{
			bestU = u;
			shortestIndex[0] = startIndex;
			shortestIndex[1] = endIndex;
			bestV = (-a).cross(params.aDir).magnitude() / ab.cross(params.aDir).magnitude();
		}
	}
	assert(bestU != FLT_MAX);
	const vector3& a = simplex.verts[shortestIndex[0]].p;
	const vector3& b = simplex.verts[shortestIndex[1]].p;
	vector3 d = params.aDir;
	vector3 a_support = params.a->Support(d, params.aTransform);
	vector3 b_support = params.b->Support(-d, params.bTransform);
	vector3 support = a_support - b_support;
	const float TOLERANCE = 0.01f;
	const bool matchesA = fabsf(support.x - a.x) < TOLERANCE && fabsf(support.y - a.y) < TOLERANCE && fabsf(support.z - a.z) < TOLERANCE;
	const bool matchesB = fabsf(support.x - b.x) < TOLERANCE && fabsf(support.y - b.y) < TOLERANCE && fabsf(support.z - b.z) < TOLERANCE;
	if (matchesA || matchesB)
	{
		outA = simplex.verts[shortestIndex[0]].A + (simplex.verts[shortestIndex[1]].A - simplex.verts[shortestIndex[0]].A) * bestV;
		outB = simplex.verts[shortestIndex[0]].B + (simplex.verts[shortestIndex[1]].B - simplex.verts[shortestIndex[0]].B) * bestV;
		outp = simplex.verts[shortestIndex[0]].p + (simplex.verts[shortestIndex[1]].p - simplex.verts[shortestIndex[0]].p) * bestV;
		outDepth = bestU / (params.aDir.magnitude());
		return true;
	}

	// insert the new point in between the two points we found
	int numVerts = (int)simplex.verts.size();
	simplex.verts.resize(numVerts + 1);
	if (shortestIndex[1] == 0)
	{
		simplex.verts[numVerts].A = a_support;
		simplex.verts[numVerts].B = b_support;
		simplex.verts[numVerts].p = support;
	}
	else
	{
		for (int i = numVerts; i > shortestIndex[1]; --i)
		{
			simplex.verts[i] = simplex.verts[i - 1];
		}
		simplex.verts[shortestIndex[1]].A = a_support;
		simplex.verts[shortestIndex[1]].B = b_support;
		simplex.verts[shortestIndex[1]].p = support;
	}
	return false;
}
//******************************************************************************
bool FindCollisionDepth(const CollisionParams& params, Simplex& simplex, float& depth, vector3& a, vector3& b)
{
	// the input is a simplex which encloses the origin
	// we need to find the minimum distance from the origin to the surrounding hull
	// of the minkowski difference.  we have not computed the whole minkowski difference
	// however (nor can we with continuous shapes)... so we iterate until we get close enough

	bool success = false;
	vector3 p;
	int iterCount = 0;
	while (iterCount < 20)
	{
		if (FindCollisionDepthStep(params, simplex, depth, a, b, p))
		{
			return true;
		}
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
    simplex.verts[0].A = params.a->Support({1,1,1}, params.aTransform);
    simplex.verts[0].B = params.b->Support({-1,-1,-1}, params.bTransform);
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

	if (outCollision)
	{
		vector3 pa, pb;
		float depth;
		outCollision->success = FindCollisionDepth(params, simplex, depth, pa, pb);
		outCollision->pointA = pa;
		outCollision->pointB = pb;
		outCollision->depth = (pb - pa).magnitude();
		outCollision->planeNormal = pa - pb;
	}

    return (result == COLLISION_RESULT_OVERLAP);
}