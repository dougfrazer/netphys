
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
static void SolveLine(Simplex& simplex, const vector3& destination)
{
    vector3 a = simplex.verts[0].p;
    vector3 b = simplex.verts[1].p;

    vector3 ap = destination - a;
    vector3 bp = destination - b;
    vector3 ab = b - a;
    vector3 ba = a - b;

    float u = bp.dot(ba);
    float v = ap.dot(ab);

    // if it is not between A and B, remove the segment it is further from.
    //           |                      |
    // region A  |       region AB      |  region B
    //           A                      B
    //  v<0      *----------------------*   u<0
    // remove b  |       keep both      |   remove a
    //           |                      |
    if (v <= 0.0f)
    {
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }
    if (u <= 0.0f)
    {
        simplex.verts[0] = simplex.verts[1];
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }
    simplex.verts[0].u = u;
    simplex.verts[1].u = v;
    simplex.divisor = ab.magnitude_sq();
    simplex.count = 2;
}
//******************************************************************************
static void SolveTriangle(Simplex& simplex, const vector3& destination)
{
    // there are 7 regions to consider:
    // point regions: a,b,c
    //  - the destination is further towards one point than the other two
    // edge regions: AB, AC, BC
    //  - the destination is further towards an edge than the other point
    // triangle region: ABC
    //  - the destination is within the triangle
    // 
    //                 \region A /
    //                  \       /
    //                   \     /
    //                    \   /
    //                     \ / A
    //                      *                   
    //         region AC   / \       region AB
    //                    /   \
    //                   /     \
    //                  /       \
    //                 /         \
    //                / region ABC\
    //               /             \
    // -------------*---------------*--------------
    //             / C              B\
    //   region C /    region BC      \ region B
    //           /                     \
    //          /                       \
    //
 
	vector3 a = simplex.verts[0].p;
	vector3 b = simplex.verts[1].p;
	vector3 c = simplex.verts[2].p;

	vector3 ao = destination - a;
	vector3 bo = destination - b;
	vector3 co = destination - c;
	vector3 ab = b - a;
	vector3 ac = c - a;
	vector3 ba = a - b;
	vector3 bc = c - b;
	vector3 ca = a - c;
	vector3 cb = b - c;

    // for each line segment, compute the dot to destination
    // see if destination is above or below each segment
    float uAB = bo.dot(ba);
	float vAB = ao.dot(ab);

    float uBC = co.dot(cb);
    float vBC = bo.dot(bc);

    float uCA = ao.dot(ac);
    float vCA = co.dot(ca);
    if (vAB <= 0.0f && uCA <= 0.0f)
    {
        // region A
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }
    if (uAB <= 0.0f && vBC <= 0.0f)
    {
        // region B
        simplex.verts[0] = simplex.verts[1];
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }
    if (vCA <= 0.0f && uBC <= 0.0f)
    {
        // region C
		simplex.verts[0] = simplex.verts[2];
		simplex.verts[0].u = 1.0f;
		simplex.divisor = 1.0f;
		simplex.count = 1;
		return;
    }

    // must be in region AB,AC,BC, or ABC

    // compute the signed triangle area
    //     Signed means that the area is positive if the vertices of the triangle 
    //     are counterclockwise, negative if the vertices are clockwise and 
    //     zero if the points are collinear
    vector3 n = ab.cross(ac);
    float area = n.magnitude_sq();

    // compare with the signed area of each edge going to the destination
    // if the signs match, then they must be in the same orientation
    // (clockwise/counterclockwise)
    float uABC = bo.cross(co).dot(n); // region BC
    float vABC = co.cross(ao).dot(n); // region AC
    float wABC = ao.cross(bo).dot(n); // region AB
    assert(FloatEquals(uABC + vABC + wABC, area));

    if (uAB > 0.0f && vAB > 0.0f && different_sign(wABC, area))
    {
        // region AB
        simplex.verts[0].u = uAB;
        simplex.verts[1].u = vAB;
        simplex.divisor = ab.magnitude_sq();
        simplex.count = 2;
        return;
    }

    if (uBC > 0.0f && vBC > 0.0f && different_sign(uABC, area))
    {
        // region BC
        simplex.verts[0] = simplex.verts[1]; // 1st->b
        simplex.verts[1] = simplex.verts[2]; // 2nd->c
        simplex.verts[0].u = uBC;
        simplex.verts[0].u = vBC;
        simplex.divisor = bc.magnitude_sq();
        simplex.count = 2;
        return;
    }

    if (uCA > 0.0f && vCA > 0.0f && different_sign(vABC, area))
    {
        simplex.verts[1] = simplex.verts[0]; // 2nd->a
        simplex.verts[0] = simplex.verts[2]; // 1st->c
        simplex.verts[0].u = uCA;
        simplex.verts[1].u = vCA;
        simplex.divisor = ca.magnitude_sq();
        simplex.count = 2;
        return;
    }

    // great, we're in region ABC, lets continue the simplex
    assert(wABC > 0.0f && vABC > 0.0f && uABC > 0.0f);
    simplex.verts[0].u = uABC;
    simplex.verts[1].u = vABC;
    simplex.verts[2].u = wABC;
    simplex.divisor = area;
    simplex.count = 3;
}
//******************************************************************************
static void SolveTetrahedron(Simplex& simplex, const vector3& destination)
{
    // 15 regions to consider:
    // 4 point regions: a,b,c,d 
    //   - the destination point is further towards a single point than any other
    // 6 edge regions: ab, ac, ad, bc, bd, cd
    //   - the destination point is further towards an edge than the other two points
    // 4 triangle regions: abc, abd, acd, bcd
    //  - the destination point is further towards a triangle than the remaining point
    // 1 tetrahedron region: abcd
    //  - the destination point is within the tetrahedron

	vector3 a = simplex.verts[0].p;
	vector3 b = simplex.verts[1].p;
	vector3 c = simplex.verts[2].p;
	vector3 d = simplex.verts[3].p;

	vector3 ap = destination - a;
	vector3 bp = destination - b;
	vector3 cp = destination - c;
	vector3 dp = destination - d;

	vector3 ab = b - a;
	vector3 ac = c - a;
    vector3 ad = d - a;

	vector3 ba = a - b;
	vector3 bc = c - b;
    vector3 bd = d - b;

	vector3 ca = a - c;
	vector3 cb = b - c;
    vector3 cd = d - c;

	vector3 da = a - d;
	vector3 db = b - d;
    vector3 dc = c - d;


	float uAB = bp.dot(ba);
	float vAB = ap.dot(ab);

	float uBC = cp.dot(cb);
	float vBC = bp.dot(bc);

	float uCA = ap.dot(ac);
	float vCA = cp.dot(ca);

    float uAD = dp.dot(da);
    float vAD = ap.dot(ad);

    float uDC = dp.dot(cd);
    float vDC = cp.dot(dc);

    float uBD = dp.dot(db);
    float vBD = bp.dot(bd);

    // point regions: A,B,C,D - is it further towards one point than any others?
    //   if so, remove other points  
    if (vAB <= 0.0f && uCA <= 0.0f && vAD <= 0.0f)
    {
        // region A
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }

    if (uAB <= 0.0f && vBC <= 0.0f && vBD <= 0.0f)
    {
        // region B
        simplex.verts[0] = simplex.verts[1];
        simplex.verts[0].u = 1.0f;
        simplex.divisor = 1.0f;
        simplex.count = 1;
        return;
    }

    if (uCA <= 0.0f && vBC <= 0.0f && vDC <= 0.0f)
    {
        // region c
		simplex.verts[0] = simplex.verts[2];
		simplex.verts[0].u = 1.0f;
		simplex.divisor = 1.0f;
		simplex.count = 1;
		return;
    }

    if (uBD <= 0.0f && vDC <= 0.0f && uAD <= 0.0f)
    {
        // region D
		simplex.verts[0] = simplex.verts[3];
		simplex.verts[0].u = 1.0f;
		simplex.divisor = 1.0f;
		simplex.count = 1;
		return;
    }
	//  there are four triangles:
    //
    //     A        A        D        B                                      
    //     *        *        *        *                                
    //    / \      / \      / \      / \                                 
    //   *---*    *---*    *---*    *---*                                          
    //   B   C    C   D    B   A    D   C                           
    //
    //  ABC, ACD, DBA, BDC
    // 
    // and six edges:
    // edge regions: AB, AC, AD, CB, CD, BD

    // to calculate if we're in an edge region, we repeat the same process for each edge:
    // 1.) check to see if its within the line segment
    //     for example: for edge AB we'd see if uAB and vAB are both positive
    //     this would mean a projection onto AB from A->destination and BA from B->destination
    // 2.) check to see if the going to and from the edge to the normals of the triangles they are on
    //     is in a counter-clockwise direction
    //     for example: a->b->normal(ABD) and b->a->normal(BAC)
    // 
    //     if it violates both of these (outside the triangles the edge is attached to) it must be
    //     in this edge region

	vector3 nABC = ab.cross(ac);
	float uABC = vector3::triple_product(b, c, nABC);
	float vABC = vector3::triple_product(c, a, nABC);
	float wABC = vector3::triple_product(a, b, nABC);

	vector3 nACD = ac.cross(ad);
	float uACD = vector3::triple_product(c, d, nACD);
	float vACD = vector3::triple_product(d, a, nACD);
	float wACD = vector3::triple_product(a, c, nACD);

	vector3 nDBA = ad.cross(ab);
	float uDBA = vector3::triple_product(b, a, nDBA);
	float vDBA = vector3::triple_product(a, d, nDBA);
	float wDBA = vector3::triple_product(d, b, nDBA);

	vector3 nBDC = bd.cross(bc);
	float uBDC = vector3::triple_product(d, c, nBDC);
	float vBDC = vector3::triple_product(c, b, nBDC);
	float wBDC = vector3::triple_product(b, d, nBDC);

    // region AB
    if (uAB > 0.0f && vAB > 0.0f && wABC <= 0.0f && uDBA <= 0.0f)
    {
        simplex.verts[0].u = uAB;
        simplex.verts[1].u = vAB;
        simplex.divisor = ab.magnitude_sq();
        simplex.count = 2;
        return;
    }

    // region AC
    if (uCA > 0.0f && vCA > 0.0f && vABC <= 0.0f && wACD <= 0.0f)
    {
		simplex.verts[1] = simplex.verts[0]; // 2nd->a
		simplex.verts[0] = simplex.verts[2]; // 1st->c
        simplex.verts[0].u = uCA;
        simplex.verts[1].u = vCA;
		simplex.divisor = ca.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region AD
    if (uAD > 0.0f && vAD > 0.0f && vACD <= 0.0f && vDBA <= 0.0f)
    {
		simplex.verts[1] = simplex.verts[3]; // 2nd->d
		simplex.verts[0].u = uAD;
		simplex.verts[1].u = vAD;
		simplex.divisor = ad.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region CB
    if (uBC > 0.0f && vBC > 0.0f && uABC <= 0.0f && vBDC <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[1]; // 1st->b
		simplex.verts[1] = simplex.verts[2]; // 2nd->c
		simplex.verts[0].u = uBC;
		simplex.verts[1].u = vBC;
		simplex.divisor = bc.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region CD
    if (uDC > 0.0f && vDC > 0.0f && uBDC <= 0.0f && uACD <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[3]; // 1st->d
		simplex.verts[1] = simplex.verts[2]; // 2nd->c
		simplex.verts[0].u = uDC;
		simplex.verts[1].u = vDC;
		simplex.divisor = dc.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region BD
    if (uBD > 0.0f && vBD > 0.0f && wDBA <= 0.0f && wBDC <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[1]; // 1st->b
		simplex.verts[1] = simplex.verts[3]; // 2nd->d
		simplex.verts[0].u = uBD;
		simplex.verts[1].u = vBD;
		simplex.divisor = bd.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // triangle region: 
	//  there are four triangles:
	//
	//     A        A        D        B                                      
	//     *        *        *        *                                
	//    / \      / \      / \      / \                                 
	//   *---*    *---*    *---*    *---*                                          
	//   B   C    C   D    B   A    D   C                           
	//
	//  ABC, ACD, DBA, BDC
    // 

    float aABCD = vector3::triple_product(bc, ba, bd);
    float uABCD = vector3::triple_product(b, d, c);
	float vABCD = vector3::triple_product(a, c, d);
	float wABCD = vector3::triple_product(d, b, a);
	float xABCD = vector3::triple_product(a, b, c);

	// region ABC
	if (xABCD <= 0.0f && uABC > 0.0f && vABC > 0.0f && wABC > 0.0f)
	{
		simplex.verts[0].u = uABC;
		simplex.verts[1].u = vABC;
		simplex.verts[2].u = wABC;
		simplex.divisor = nABC.magnitude_sq();
		assert(FloatEquals(uABC + vABC + wABC, nABC.magnitude_sq()));
		simplex.count = 3;
		return;
	}

	// region ACD
	if (vABCD <= 0.0f && uACD > 0.0f && vACD > 0.0f && wACD > 0.0f)
	{
		simplex.verts[1] = simplex.verts[2]; // 2nd->c
		simplex.verts[2] = simplex.verts[3]; // 3rd->d
		simplex.verts[0].u = uACD;
		simplex.verts[1].u = vACD;
		simplex.verts[2].u = wACD;
		simplex.divisor = nACD.magnitude_sq();
		assert(FloatEquals(uACD + vACD + wACD, nACD.magnitude_sq()));
		simplex.count = 3;
		return;
	}

	// region DBA
	if (wABCD <= 0.0f && uDBA > 0.0f && vDBA > 0.0f && wDBA > 0.0f)
	{
		simplex.verts[2] = simplex.verts[0]; // 3rd->a
		simplex.verts[0] = simplex.verts[3]; // 1st->d
		simplex.verts[0].u = uDBA;
		simplex.verts[1].u = vDBA;
		simplex.verts[2].u = wDBA;
		simplex.divisor = nDBA.magnitude_sq();
		assert(FloatEquals(uDBA + vDBA + wDBA, nDBA.magnitude_sq()));
		simplex.count = 3;
		return;
	}

    // region BDC
    if (uABCD <= 0.0f && uBDC > 0.0f && vBDC > 0.0f && wBDC > 0.0f)
    {
        simplex.verts[0] = simplex.verts[1]; // 1st->b
        simplex.verts[1] = simplex.verts[3]; // 2nd->d
        simplex.verts[2] = simplex.verts[2]; // 3rd->c
        simplex.verts[0].u = uBDC;
        simplex.verts[1].u = vBDC;
        simplex.verts[2].u = wBDC;
        simplex.divisor = nBDC.magnitude_sq();
        assert(FloatEquals(uBDC + vBDC + wBDC, nBDC.magnitude_sq()));
        simplex.count = 3;
        return;
    }

	// otherwise, we must be inside ABCD
    assert(uABCD > 0.0f && vABCD > 0.0f && wABCD > 0.0f && xABCD > 0.0f);
    simplex.verts[0].u = uABCD;
    simplex.verts[1].u = vABCD;
    simplex.verts[2].u = wABCD;
    simplex.verts[3].u = xABCD;
    simplex.divisor = aABCD;
	//assert(FloatEquals(uABCD + vABCD + wABCD + xABCD, aABCD));
    simplex.count = 4;
}
//******************************************************************************
static const vector3 up = { 0.0f, 1.0f, 0.0f };
//******************************************************************************
vector3 GetSearchDirection(const Simplex& simplex, const vector3& destination)
{
    if (simplex.count == 1)
    {
        return -simplex.verts[0].p;
    }
    else if (simplex.count == 2)
    {
        const vector3& a = simplex.verts[0].p;
        const vector3& b = simplex.verts[1].p;
        vector3 ab = b - a;
        vector3 bo = -b;
        vector3 t = ab.cross(bo);
        return t.cross(ab);
    }
    else if (simplex.count == 3)
    {
        const vector3& a = simplex.verts[0].p;
        const vector3& b = simplex.verts[1].p;
        const vector3& c = simplex.verts[2].p;
        vector3 ab = b - a;
        vector3 ac = c - a;
        vector3 n = ab.cross(ac);
        float sign = n.dot(a);
        if (sign <= 0.0f)
		{
			return n;
		}
		else
		{
			return -n;
		}
    }

	assert(false);
	return vector3();
}
//******************************************************************************
void GetWitnessPoints(const Simplex& s, vector3& outA, vector3& outB)
{
    switch (s.count)
    {
        case 1:
        {
            outA = s.verts[0].A;
            outB = s.verts[0].B;
        }
        break;

        case 2:
        {
            float t = 1.0f / s.divisor;
            outA = s.verts[0].A * (t * s.verts[0].u) + s.verts[1].A * (t * s.verts[1].u);
            outB = s.verts[0].B * (t * s.verts[0].u) + s.verts[1].B * (t * s.verts[1].u);
        }
        break;
 
        case 3:
        {
			float t = 1.0f / s.divisor;
			outA = s.verts[0].A * (t * s.verts[0].u) + s.verts[1].A * (t * s.verts[1].u) + s.verts[2].A * (t * s.verts[2].u);
			outB = s.verts[0].B * (t * s.verts[0].u) + s.verts[1].B * (t * s.verts[1].u) + s.verts[2].B * (t * s.verts[2].u);
        }
        break;

        case 4:
        {
			float t = 1.0f / s.divisor;
			outA = s.verts[0].A * (t * s.verts[0].u) + s.verts[1].A * (t * s.verts[1].u) + s.verts[2].A * (t * s.verts[2].u) + s.verts[3].A * (t * s.verts[3].u);
			outB = s.verts[0].B * (t * s.verts[0].u) + s.verts[1].B * (t * s.verts[1].u) + s.verts[2].B * (t * s.verts[2].u) + s.verts[3].B * (t * s.verts[3].u);
        }
        break;

        default: assert(false); break;
    }
}
//******************************************************************************
COLLISION_STEP DetectCollisionStep(const CollisionParams& params, Simplex& simplex, const vector3& destination)
{
	Simplex save = simplex;
	switch (simplex.count)
	{
		case 1:
			break;

		case 2:
			SolveLine(simplex, destination);
			break;

		case 3:
			SolveTriangle(simplex, destination);
			break;

		case 4:
			SolveTetrahedron(simplex, destination);
			break;

		default:
			assert(false);
	}

	if (simplex.count == 4)
	{
		return COLLISION_STEP_SUCCESS; // done
	}

	vector3 d = GetSearchDirection(simplex, destination);
	if (FloatEquals(d.dot(d), 0.0f))
	{
		return COLLISION_STEP_FAILURE;
	}

	vector3 a_support = params.a->Support(d, params.aTransform);
	vector3 b_support = params.b->Support(-d, params.bTransform);
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_STEP_FAILURE;
	}
	for (int i = 0; i < save.count; ++i)
	{
		if (a_support == simplex.verts[i].A && b_support == simplex.verts[i].B)
		{
			return COLLISION_STEP_FAILURE;
		}
	}
	simplex.verts[simplex.count].A = a_support;
	simplex.verts[simplex.count].B = b_support;
	simplex.verts[simplex.count].p = support;
	simplex.count++;
    return COLLISION_STEP_CONTINUE;
}

//******************************************************************************
bool DetectCollision(const CollisionParams& params, CollisionData* outCollision)
{
    Simplex simplex;

    // TODO: broad phase

    // start with any point in the geometries
    simplex.verts[0].A = params.aTransform * params.a->GetRandomPointOnEdge();
    simplex.verts[0].B = params.bTransform * params.b->GetRandomPointOnEdge();
    simplex.verts[0].p = simplex.verts[0].A - simplex.verts[0].B;
    simplex.count = 1;

    const vector3& destination = {0.0f, 0.0f, 0.0f}; // origin
    constexpr int maxIterations = 20;
    int iterCount = 0;
    COLLISION_STEP step = COLLISION_STEP_CONTINUE;
    while (step == COLLISION_STEP_CONTINUE && iterCount++ < maxIterations)
    {
        step = DetectCollisionStep(params, simplex, destination);
    }
    assert(iterCount < maxIterations); // if we bailed due to iterations... we have undefined collision
    assert(step != COLLISION_STEP_CONTINUE);

    // no overlap
    if (step == COLLISION_STEP_FAILURE)
    {
        return false;
    }

    if (outCollision)
    {
        vector3 pa,pb;
        GetWitnessPoints(simplex, pa, pb);
        outCollision->point = pa;
        outCollision->depth = (pb - pa).magnitude();
        outCollision->planeNormal = {0.0f,1.0f,0.0f};
        //outCollision->planeNormal = (pb - pa).cross(up).normalize();
    }
    return true;
}