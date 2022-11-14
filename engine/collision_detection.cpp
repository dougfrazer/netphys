
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

static const vector3 origin;

//******************************************************************************
// Support:
// --------
//   This will attempt to find the closest point in the geometry to the vector
//   passed in.
//******************************************************************************;
static vector3 Support(const vector3& d, const Geometry* Geo, const matrix4* m);
static SimplexPoint Support(const vector3& d, const Geometry* A, const matrix4* at, const Geometry* B, const matrix4* bt);

//******************************************************************************
// SimplexIter:
// --------
//   This function will take in a list of points, and a direction.  It knows the last
//   point was a point that was just added.  We will test to see if that point is part of
//   a tetrahedron surrounding the origin.  If it is not, we discard it, and change the direction to face
//   the origin.  If it is part of the tetrahedron, we leave it, and update our search direction
//   towards the origin but in the direction perpendicular to what we just added.
//
// Note: this function will assume the last point in the list be the point we just added
// Note: this function modifies both the list and the direction.
//******************************************************************************
static bool SimplexIter(Simplex* s, vector3* d, const vector3& dest);


// todo hack: just assuming the one board at 0,0,0 is the only plane.
static     vector3 s_planeNormal = { 0.0f, 1.0f, 0.0f };
static     vector3 s_planePoint = { 0.0f, 0.0f, 0.0f };
//******************************************************************************
static vector3 GetCollisionPlaneNormal(const Physics* a, const Physics* b)
{
    // todo
    return s_planeNormal;
}
//******************************************************************************
//static vector3 GetCollisionPoint(const Physics* a, const Physics* b, const vector3& planeNormal, float* depth)
//{
//    matrix4 m = a->GetTransform();
//
//    float min = std::numeric_limits<float>::infinity();
//    vector3 ret;
//    for (int i = 0; i < a->GetGeometry()->m_numVertices; i++)
//    {
//        vector3 current = vector3(m * vector4(a->GetGeometry()->m_vertexList[i]));
//        float d = current.dot(planeNormal);
//        if (d < min)
//        {
//            min = d;
//            ret = current;
//        }
//    }
//    assert(min != std::numeric_limits<float>::infinity());
//    if (depth)
//    {
//        *depth = fabs(min);
//    }
//    return ret;
//}
//******************************************************************************
void GetCollisionPoint(const Simplex& simplex, vector3& pointA, vector3& pointB, float& depth)
{
    // the simplex represents the best volume we could create to enclose the origin
    // to determine an overlap.  4 points means there is an overlap, 2 or 3 means
    // there is no overlap, but we can determine the closest points to overlapping.
    if(simplex.count == 2)
    {
        // closest point on a line to origin
        // NOTE: this is untested!!!  verify before using
        const vector3& a = simplex.verts[0].p;
        const vector3& b = simplex.verts[1].p;
        float a_ratio = ClosestPointLinePointRatio(origin, a, b);
        assert(a_ratio >= 0.0f && a_ratio <= 1.0f); // not sure this is valid, but ranges outside mean the closest point is outside the range of the vertices... dont think thats possible?
        float b_ratio = 1.0f - a_ratio;
        pointA = simplex.verts[0].A * a_ratio + simplex.verts[1].A * a_ratio;
        pointB = simplex.verts[0].B * b_ratio + simplex.verts[1].B * b_ratio;
    }
    else if(simplex.count == 3)
    {
        // closest point on a triangle to origin
        // NOTE: this is untested!!!  verify before using
		const vector3& a = simplex.verts[0].p;
		const vector3& b = simplex.verts[1].p;
		const vector3& c = simplex.verts[2].p;
        EdgeRatios st = ClosestPointTrianglePointRatio(origin, a, b, c);
        pointA = simplex.verts[0].A + simplex.verts[1].A * st.s + simplex.verts[2].A * st.t;
        pointB = simplex.verts[0].B + simplex.verts[1].B * st.s + simplex.verts[2].B * st.t;
    }
    else if(simplex.count == 4)
    {
		const SimplexPoint& s_a = simplex.verts[0];
		const SimplexPoint& s_b = simplex.verts[1];
		const SimplexPoint& s_c = simplex.verts[2];
		const SimplexPoint& s_d = simplex.verts[3];
		const vector3& a = s_a.p;
		const vector3& b = s_b.p;
		const vector3& c = s_c.p;
		const vector3& d = s_d.p;
		const vector3& p = origin;

        vector3 ab = b - a;
        vector3 ac = c - a;
        vector3 ad = d - a;

        float volume = 1.0f / vector3::box(ab, ac, ad);
        float u_abcd = vector3::box(c,d,b) * volume;
        float v_abcd = vector3::box(c,a,d) * volume;
        float w_abcd = vector3::box(d,a,b) * volume;
        float x_abcd = vector3::box(b,a,c) * volume;

        float denom = 1.0f / (u_abcd + v_abcd + w_abcd + x_abcd);

        vector3 i = s_a.A * u_abcd * denom;
        vector3 j = s_b.A * v_abcd * denom;
        vector3 k = s_c.A * w_abcd * denom;
        vector3 l = s_d.A * x_abcd * denom;

        pointA = i + j + k + l;
        pointB = pointA;
        depth = pointA.magnitude();
    #if 0
		const SimplexPoint& s_a = simplex.verts[0];
		const SimplexPoint& s_b = simplex.verts[1];
		const SimplexPoint& s_c = simplex.verts[2];
		const SimplexPoint& s_d = simplex.verts[3];
		const vector3& a = s_a.p;
		const vector3& b = s_b.p;
		const vector3& c = s_c.p;
		const vector3& d = s_d.p;
		const vector3& p = origin;


        vector3 ab = a - b;
        vector3 ba = b - a;
        vector3 bc = b - c;
        vector3 cb = c - b;
        vector3 ca = c - a;
        vector3 ac = a - c;
        vector3 db = d - b;
        vector3 bd = b - d;
        vector3 dc = d - c;
        vector3 cd = c - d;
        vector3 da = d - a;
        vector3 ad = a - d;

        float u_ab = b.dot(ba);
        float v_ab = a.dot(ab);

        float u_bc = c.dot(cb);
        float v_bc = b.dot(bc);
        
        float u_ca = a.dot(ac);
        float v_ca = c.dot(ca);

        float u_bd = d.dot(db);
        float v_db = b.dot(bd);

        float u_dc = c.dot(cd);
        float v_dc = d.dot(dc);

        float u_ad = d.dot(da);
        float v_ad = a.dot(ad);

		/* calculate fractional area */
		vector3 n = da.cross(ba);
        vector3 n1 = d.cross(b);
        vector3 n2 = b.cross(a);
        vector3 n3 = a.cross(d);

        float u_abd = n1.dot(n);

		float u_adb = n1.dot(n);
		float v_adb = n2.dot(n);
		float w_adb = n3.dot(n);

		f3cross(n, ca, da);
		f3cross(n1, c, d);
		f3cross(n2, d, a);
		f3cross(n3, a, c);

		float u_acd = f3dot(n1, n);
		float v_acd = f3dot(n2, n);
		float w_acd = f3dot(n3, n);

		f3cross(n, bc, dc);
		f3cross(n1, b, d);
		f3cross(n2, d, c);
		f3cross(n3, c, b);

		float u_cbd = f3dot(n1, n);
		float v_cbd = f3dot(n2, n);
		float w_cbd = f3dot(n3, n);

		f3cross(n, ba, ca);
		f3cross(n1, b, c);
		f3cross(n2, c, a);
		f3cross(n3, a, b);

		float u_abc = f3dot(n1, n);
		float v_abc = f3dot(n2, n);
		float w_abc = f3dot(n3, n);
    #endif
    #if 0
        // closest point on a tetrahedron to origin
		const SimplexPoint& s_a = simplex.verts[0];
		const SimplexPoint& s_b = simplex.verts[1];
		const SimplexPoint& s_c = simplex.verts[2];
		const SimplexPoint& s_d = simplex.verts[3];
		const vector3& a = s_a.p;
		const vector3& b = s_b.p;
		const vector3& c = s_c.p;
		const vector3& d = s_d.p;
        const vector3& p = origin;

		vector3 ab = b - a;
		vector3 ac = c - a;
		vector3 ad = d - a;
		vector3 bc = c - b;
		vector3 bd = d - b;

		// normals of each of the faces of the tetrahedron
		vector3 n1 = ab.cross(ac);
		vector3 n2 = ab.cross(ad);
		vector3 n3 = ac.cross(ad);
		vector3 n4 = bc.cross(bd);

		// center of the tetrahedron
		vector3 center = (n1 + n2 + n3 + n4) / 4;

		// find the projection along the normal from the origin to the
		// center... the largest projection will tell us the closest face
		vector3 pc = p - center;
		float d1 = pc.dot(n1);
		float d2 = pc.dot(n2);
		float d3 = pc.dot(n3);
		float d4 = pc.dot(n4);

        EdgeRatios st;
		if (d1 > d2 && d1 > d3 && d1 > d4)
		{
			// face 1 is closest face to origin
			st = ClosestPointTrianglePointRatio(p, a, b, c);
			pointA = s_a.A + (s_b.A - s_a.A) * st.s + (s_c.A - s_a.A) * st.t;
			pointB = s_a.B + (s_b.B - s_a.B) * st.s + (s_c.B - s_a.B) * st.t;
			vector3 point = a + (b - a) * st.s + (c - a) * st.t;
			depth = point.magnitude();
		}
		else if (d2 > d1 && d2 > d3 && d2 > d4)
		{
			// face 2 is closest face
            st = ClosestPointTrianglePointRatio(p, a, b, d);
			pointA = s_a.A + (s_b.A - s_a.A) * st.s + (s_d.A - s_a.A) * st.t;
			pointB = s_a.B + (s_b.B - s_a.B) * st.s + (s_d.B - s_a.B) * st.t;
			vector3 point = a + (b - a) * st.s + (d - a) * st.t;
			depth = point.magnitude();
		}
		else if (d3 > d1 && d3 > d2 && d3 > d4)
		{
			// face 3 is closest face
            st = ClosestPointTrianglePointRatio(p, a, c, d);
			pointA = s_a.A + (s_c.A - s_a.A) * st.s + (s_d.A - s_a.A) * st.t;
			pointB = s_a.B + (s_c.B - s_a.B) * st.s + (s_d.B - s_a.B) * st.t;
			vector3 point = a + (c - a) * st.s + (d - a) * st.t;
			depth = point.magnitude();
		}
		else if (d4 > d1 && d4 > d2 && d4 > d3)
		{
			// face 4 is closest face
            st = ClosestPointTrianglePointRatio(p, b, c, d);
			pointA = s_b.A + (s_c.A - s_b.A) * st.s + (s_d.A - s_b.A) * st.t;
			pointB = s_b.B + (s_c.B - s_b.B) * st.s + (s_d.B - s_b.B) * st.t;
            vector3 point = b + (c-b)*st.s + (d-c)*st.t;
            depth = point.magnitude();
		}
		else
		{
			assert(false); // impossible?
		}
    #endif
    }
    else
    {
        assert(false); // impossible
    }
}


#if 0
//******************************************************************************
bool DetectCollision(const Physics* a, const Physics* b, CollisionData* outCollision)
{
    Simplex simplex;

    // TODO: broad phase

    // start with any point in the mikowski difference
    vector3 start((vector3(a->GetGeometry()->m_vertexList[0]) + vector3(a->GetPosition())) -
                  (vector3(b->GetGeometry()->m_vertexList[0]) + vector3(b->GetPosition())));
    simplex.verts[0] = {a->GetGeometry()->m_vertexList[0], b->GetGeometry()->m_vertexList[0], start};
    simplex.count = 1;

    // go towards the origin
    vector3 d = -start;
    while (true)
    {
        matrix4 a_transform = a->GetTransform();
        matrix4 b_transform = b->GetTransform();
        SimplexPoint support = Support(d, a->GetGeometry(), &a_transform, b->GetGeometry(), &b_transform);
        if (d.dot(support.p) < 0)
        {
            return false;
        }
        simplex.verts[simplex.count] = support;
        simplex.count++;
        if (SimplexIter(&simplex, &d, origin))
        {
            if (outCollision)
            {
                outCollision->planeNormal = GetCollisionPlaneNormal(a, b);
                float collisionDepth = 0.0f;
                vector3 a,b;
                GetCollisionPoint(simplex, a, b, collisionDepth);
                outCollision->point = a;
                outCollision->depth = collisionDepth;
            }
            return true;
        }
    }

    return false;
}

//******************************************************************************
// Find the furthest point in the minkowski difference of A and B in the direction d.
// This is the same as saying find the furthest point in A in the direction of d
// MINUS the furthest point in B in the opposite direction
//******************************************************************************
SimplexPoint Support(const vector3& d, const Geometry* A, const matrix4* A_transform, const Geometry* B, const matrix4* B_transform)
{
    vector3 a_support = Support(d, A, A_transform);
    vector3 b_support = Support(-d, B, B_transform);
    return SimplexPoint({a_support, b_support, a_support - b_support});
}
//******************************************************************************
// find the furthest point in the geometry with transform matrix m in
// the direction d
//******************************************************************************
vector3 Support(const vector3& d, const Geometry* Geo, const matrix4* const m)
{
    float dot_product = -std::numeric_limits<float>::infinity();
    vector3 closest;
    for (int i = 0; i < Geo->m_numVertices; i++)
    {
        vector3 t = vector3(*m * vector4(Geo->m_vertexList[i]));

        float c = d.dot(t);
        if (c >= dot_product)
        {
            dot_product = c;
            closest = t;
        }
    }
    assert(dot_product != -std::numeric_limits<float>::infinity());
    return closest;
}
//******************************************************************************

//******************************************************************************
bool SimplexIter(Simplex* simplex, vector3* dir, const vector3& dest)
{
    if (simplex->count == 2)
    {
        vector3 a = simplex->verts[1].p;
        vector3 b = simplex->verts[0].p;
        vector3 ab = b - a;
        vector3 ao = dest - a;
        if (ab.dot(ao) > 0)
        {
            *dir = ab.cross(ao).cross(ab);
        }
        else
        {
            *dir = ao;
            simplex->count = 1;
        }
    }
    else if (simplex->count == 3)
    {
        // we have a triangle/plane:
        SimplexPoint a = simplex->verts[2];
        SimplexPoint b = simplex->verts[1];
        SimplexPoint c = simplex->verts[0];
        vector3 ab = b.p - a.p;
        vector3 ac = c.p - a.p;
        vector3 abc = ab.cross(ac);
        vector3 ao = dest - a.p;

        // first check to see if the point is above AC
        if (abc.cross(ac).dot(ao) > 0)
        {
            if (ac.dot(ao) > 0)
            {
                *dir = ac.cross(ao).cross(ac);
                simplex->verts[0] = a;
                simplex->verts[1] = c;
                simplex->count = 2;
            }
            else if (ab.dot(ao))
            {
                *dir = ab.cross(ao).cross(ab);
                simplex->verts[0] = a;
                simplex->verts[1] = b;
                simplex->count = 2;
            }
            else
            {
                *dir = ao;
                simplex->verts[0] = a;
                simplex->count = 1;
            }
            // then check to see if the point is below AB
        }
        else if (ab.cross(abc).dot(ao) > 0)
        {
            if (ab.dot(ao) > 0)
            {
                *dir = ab.cross(ao).cross(ab);
                simplex->verts[0] = a;
                simplex->verts[1] = b;
                simplex->count = 2;
            }
            else if (ab.dot(ao))
            {
                *dir = ac.cross(ao).cross(ac);
                simplex->verts[0] = a;
                simplex->verts[1] = c;
                simplex->count = 2;
            }
            else
            {
                *dir = ao;
                simplex->verts[0] = a;
                simplex->count = 1;
            }
            // OK, the point is necessarily below AC and above AB (so it is within the triangle),
            // just need to figure out which direction to search
        }
        else if (abc.dot(ao) > 0)
        {
            *dir = abc;
            simplex->verts[0] = a;
            simplex->verts[1] = b;
            simplex->verts[2] = c;
            simplex->count = 3;
        }
        else
        {
            *dir = -abc;
            simplex->verts[0] = a;
            simplex->verts[1] = c;
            simplex->verts[2] = b;
            simplex->count = 3;
        }
    }
    else if (simplex->count == 4)
    {
        // we have a tetrahedron
        // return whether or not the origin is enclosed in the tetrahedron
        SimplexPoint a = simplex->verts[3];
        SimplexPoint b = simplex->verts[2];
        SimplexPoint c = simplex->verts[1];
        SimplexPoint d = simplex->verts[0];
        vector3 ab = b.p - a.p;
        vector3 ac = c.p - a.p;
        vector3 ad = d.p - a.p;
        vector3 ao = dest - a.p;
        vector3 abd = ab.cross(ad);
        vector3 adc = ad.cross(ac);
        vector3 acb = ac.cross(ab);
        if (abd.dot(ao) > 0)
        {
            *dir = abd;
            simplex->verts[0] = a;
            simplex->verts[1] = b;
            simplex->verts[2] = d;
            simplex->count = 3;
        }
        else if (adc.dot(ao) > 0)
        {
            *dir = adc;
            simplex->verts[0] = a;
            simplex->verts[1] = d;
            simplex->verts[2] = c;
            simplex->count = 3;
        }
        else if (acb.dot(ao) > 0)
        {
            *dir = acb;
            simplex->verts[0] = a;
            simplex->verts[1] = c;
            simplex->verts[2] = b;
            simplex->count = 3;
        }
        else
        {
            return true;
        }
    }
    else
    {
        assert(false);  // should not be possible
    }
    return false;
}
//******************************************************************************
#endif





























//******************************************************************************
void SolveLine(Simplex& simplex, const vector3& destination)
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
void SolveTriangle(Simplex& simplex, const vector3& destination)
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
   // assert(FloatEquals(uABC + vABC + wABC, area));

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
        simplex.verts[0] = simplex.verts[1];
        simplex.verts[1] = simplex.verts[2];
        simplex.verts[0].u = uBC;
        simplex.verts[0].u = vBC;
        simplex.divisor = bc.magnitude_sq();
        simplex.count = 2;
        return;
    }

    if (uCA > 0.0f && vCA > 0.0f && different_sign(vABC, area))
    {
        simplex.verts[1] = simplex.verts[0];
        simplex.verts[0] = simplex.verts[2];
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
void SolveTetrahedron(Simplex& simplex, const vector3& destination)
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
    //     A        A        A        C                                      
    //     *        *        *        *                                
    //    / \      / \      / \      / \                                 
    //   *---*    *---*    *---*    *---*                                          
    //   D   B    D   C    C   B    D   B                           
    //
    //  DBA, DCA, CBA, DBC
    // 
    // and six edges:
    // edge regions: AB, AC, AD, CB, CD, BD

    // to calculate if we're in an edge region, we repeat the same process for each edge:
    // 1.) check to see if its within the line segment
    //     for example: for edge AB we'd see if uAB and vAB are both positive
    //     this would mean a projection onto AB from AO and BA from BO
    // 2.) check to see if the path to the going both directions along the line to the destination
    //     are both the same orientation
    //     for example: if ABO and BAO are both negative
    // 
    //     if they both are, then we know we should eliminate the other 2 vertices
    //     as the destination is furthest in the direction of this edge

	vector3 nADB = ad.cross(ab);
	float uADB = vector3::triple_product(d, b, nADB);
	float vADB = vector3::triple_product(b, a, nADB);
	float wADB = vector3::triple_product(a, d, nADB);

	vector3 nACD = ac.cross(ad);
	float uACD = vector3::triple_product(c, d, nACD);
	float vACD = vector3::triple_product(d, a, nACD);
	float wACD = vector3::triple_product(a, c, nACD);

	vector3 nCBD = cb.cross(cd);
	float uCBD = vector3::triple_product(b, d, nCBD);
	float vCBD = vector3::triple_product(d, c, nCBD);
	float wCBD = vector3::triple_product(c, b, nCBD);

	vector3 nABC = ab.cross(ac);
	float uABC = vector3::triple_product(b, c, nABC);
	float vABC = vector3::triple_product(c, a, nABC);
	float wABC = vector3::triple_product(a, b, nABC);

    // region AB
    if (uAB > 0.0f && vAB > 0.0f && vABC <= 0.0f && wADB <= 0.0f)
    {
        simplex.verts[0].u = uAB;
        simplex.verts[1].u = vAB;
        simplex.divisor = ab.magnitude_sq();
        simplex.count = 2;
        return;
    }

    // region AC
    if (uCA > 0.0f && vCA > 0.0f && wABC <= 0.0f && vACD <= 0.0f)
    {
		simplex.verts[1] = simplex.verts[0]; // b = a
		simplex.verts[0] = simplex.verts[2]; // a = c
        simplex.verts[0].u = uCA;
        simplex.verts[1].u = vCA;
		simplex.divisor = ca.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region AD
    if (uAD > 0.0f && vAD > 0.0f && vADB <= 0.0f && wACD <= 0.0f)
    {
		simplex.verts[1] = simplex.verts[3]; // b = d
		simplex.verts[0].u = uAD;
		simplex.verts[1].u = vAD;
		simplex.divisor = ad.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region CB
    if (uBC > 0.0f && vBC > 0.0f && uABC <= 0.0f && vCBD <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[1]; // a = b
		simplex.verts[1] = simplex.verts[2]; // b = c
		simplex.verts[0].u = uBC;
		simplex.verts[1].u = vBC;
		simplex.divisor = bc.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region CD
    if (uDC > 0.0f && vDC > 0.0f && wCBD <= 0.0f && uACD <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[3]; // a = d
		simplex.verts[1] = simplex.verts[2]; // b = c
		simplex.verts[0].u = uDC;
		simplex.verts[1].u = vDC;
		simplex.divisor = dc.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // region BD
    if (uBD > 0.0f && vBD > 0.0f && uADB <= 0.0f && uCBD <= 0.0f)
    {
		simplex.verts[0] = simplex.verts[1]; // a = b
		simplex.verts[1] = simplex.verts[3]; // b = d
		simplex.verts[0].u = uBD;
		simplex.verts[1].u = vBD;
		simplex.divisor = bd.magnitude_sq();
		simplex.count = 2;
		return;
    }

    // triangle region: 
	//     there are four triangles:
    //
	//     A        A        A        C                                      
	//     *        *        *        *                                
	//    / \      / \      / \      / \                                 
	//   *---*    *---*    *---*    *---*                                          
	//   D   B    C   D    B   C    B   D                           
    //
    //  ADB, ACD, CDB, ABC
    // 

    float aABCD = vector3::triple_product(bc, ba, bd);
    float uABCD = vector3::triple_product(c, d, b);
	float vABCD = vector3::triple_product(c, a, d);
	float wABCD = vector3::triple_product(d, a, b);
	float xABCD = vector3::triple_product(b, a, c);

    // region DBA
    if (uABCD <= 0.0f && uADB > 0.0f && vADB > 0.0f && wADB > 0.0f)
    {
        simplex.verts[2] = simplex.verts[3]; // c = d
        simplex.verts[0].u = uADB;
        simplex.verts[1].u = vADB;
        simplex.verts[2].u = wADB;
        simplex.divisor = nADB.magnitude_sq();
        //assert(FloatEquals(uDBA+vDBA+wDBA,nDBA.magnitude_sq());
        simplex.count = 3;
        return;
    }

    // region DCA
    if (vABCD <= 0.0f && uACD > 0.0f && vACD > 0.0f && wACD > 0.0f)
    {
		simplex.verts[1] = simplex.verts[3]; // b = d
		simplex.verts[0].u = uACD;
		simplex.verts[1].u = vACD;
		simplex.verts[2].u = wACD;
		simplex.divisor = nACD.magnitude_sq();
        // assert(FloatEquals(uDCA+vDCA+wDCA,nDCA.magnitude_sq());
		simplex.count = 3;
		return;
    }

    // region DBC
    if (xABCD <= 0.0f && uABC > 0.0f && vABC > 0.0f && wABC > 0.0f)
    {
		simplex.verts[3] = simplex.verts[1]; // copy b into d
		simplex.verts[1] = simplex.verts[2]; // b = c
		simplex.verts[2] = simplex.verts[3]; // c = b (copy in d)
		simplex.verts[0].u = uABC;
		simplex.verts[1].u = vABC;
		simplex.verts[2].u = wABC;
		simplex.divisor = nABC.magnitude_sq();
        // assert(FloatEquals(uCBA+vCBA+wCBA,nCBA.magnitude_sq());
		simplex.count = 3;
		return;
    }

    // region CBA
    if (wABCD <= 0.0f && uCBD > 0.0f && vCBD > 0.0f && wCBD > 0.0f)
    {
		simplex.verts[0] = simplex.verts[2]; // a = c
		simplex.verts[2] = simplex.verts[1]; // c = b
		simplex.verts[1] = simplex.verts[3]; // b = d
		simplex.verts[0].u = uCBD;
		simplex.verts[1].u = vCBD;
		simplex.verts[2].u = wCBD;
		simplex.divisor = nCBD.magnitude_sq();
        // assert(FloatEquals(uDBC+vDBC+wDBC,nDBC.magnitude_sq());
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
    // assert(FloatEquals(uABCD+vABCD+wABCD+xABCD,aABCD);
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
        vector3 ab = simplex.verts[1].p - simplex.verts[0].p;
        vector3 n = up.cross(ab);
        float sign = n.dot(destination - simplex.verts[0].p);
        if (sign > 0.0f)
        {
            return n;
        }
        else
        {
            return -n;
        }
    }
    else if (simplex.count == 3)
    {
        vector3 ab = simplex.verts[1].p - simplex.verts[0].p;
        vector3 ac = simplex.verts[2].p - simplex.verts[0].p;
        vector3 n = ab.cross(ac);
        float sign = n.dot(destination - simplex.verts[0].p);
		if (sign > 0.0f)
		{
			return n;
		}
		else
		{
			return -n;
		}
    }
    else
    {
        assert(false);
        return vector3();
    }
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
int Support(const vector3& dir, const Geometry* geo, const vector3& trans)
{
    // get the position furthest in the direction of geo (modified by trans)
	float dot_product = -std::numeric_limits<float>::infinity();
	int closestIndex;
    // todo: do we need to consider all vertices, or can we iteratively solve for the furthest?
	for (int i = 0; i < geo->m_numVertices; i++)
	{
		vector3 t = trans + geo->m_vertexList[i];

		float c = dir.dot(t);
		if (c >= dot_product)
		{
			dot_product = c;
            closestIndex = i;
		}
	}
	assert(dot_product != -std::numeric_limits<float>::infinity());
	return closestIndex;
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

	int a_support_index = Support(d, params.a, params.aPos);
	int b_support_index = Support(-d, params.b, params.bPos);
	vector3 a_support = params.aPos + params.a->m_vertexList[a_support_index];
	vector3 b_support = params.bPos + params.b->m_vertexList[b_support_index];
	vector3 support = a_support - b_support;
	if (d.dot(support) < 0)
	{
		return COLLISION_STEP_FAILURE;
	}
	for (int i = 0; i < save.count; ++i)
	{
		if (a_support_index == simplex.verts[i].indexA && b_support_index == simplex.verts[i].indexB)
		{
			return COLLISION_STEP_FAILURE;
		}
	}
	simplex.verts[simplex.count].indexA = a_support_index;
	simplex.verts[simplex.count].indexB = b_support_index;
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
    vector3 a_local = vector3(params.a->m_vertexList[0]);
    vector3 b_local = vector3(params.b->m_vertexList[0]);
    simplex.verts[0].A = a_local + params.aPos;
    simplex.verts[0].B = b_local + params.bPos;
    simplex.verts[0].indexA = 0;
    simplex.verts[0].indexB = 0;
    simplex.verts[0].p = simplex.verts[0].A - simplex.verts[0].B;
    simplex.count = 1;

    const vector3& destination = origin;
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
        outCollision->planeNormal = (pb - pa).normalize();
    }
    return true;
}