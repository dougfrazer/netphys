#pragma once

#include "vector.h"


//  
//           |                      |
// region A  |       region AB      |  region B
//           A                      B
//  u<0      *----------------------*   u>1
//           |                      |
enum LineRegion
{
	LineRegion_AB = 0,
	LineRegion_A,
	LineRegion_B,
};
inline LineRegion ClosestPoint_LinePointRatio(const vector3& p, const vector3& a, const vector3& b, float& u)
{
	const vector3 ap = p - a;
	const vector3 ab = b - a;

	u = ap.dot(ab) / ab.dot(ab);

	if (u <= 0.0f)
	{
		u = 0.0f;
		return LineRegion_A;
	}
	if (u >= 1.0f)
	{
		u = 1.0f;
		return LineRegion_B;
	}

	return LineRegion_AB;
}

inline vector3 ClosestPoint_LinePoint(const vector3& p, const vector3& a, const vector3& b)
{
	float u;
	ClosestPoint_LinePointRatio(p, a, b, u);
	return a + (b - a) * u;
}

inline float LinePointDistanceSq(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 c = ClosestPoint_LinePoint(p, a, b);
	return (p - c).magnitude_sq();
}

inline float LinePointDistance(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 c = ClosestPoint_LinePoint(p, a, b);
	return (p - c).magnitude();
}

//
//  See: https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
// 
//  Any point on triangle can be represented as
//    T(u,v) = A + u(B-A) + v(C-A)
//	where A,B,C form a valid triangle
//  that is to say: any point on the triangle can be represented as 
//  one point in the triangle plus some scalar times each of the edges
//
//  Furthermore, we can represent the distance between a point on the triangle
//  and an arbitrary point P as:
//    Q(u,v) = | T(u,v) - P |
//  or, as the squared distance, which correlates directly to the distance:
//    Q'(u,v) = | T(u,v) - P |^2
//  which we can expand using the dot product identity ( a dot a = |a|^2 )
//    Q'(u,v) = (T(u,v) - P) dot (T(u,v) - P)
//   and further expand due to its distributive property
//            = T(u,v) dot (T(u,v) - P)         -      P dot (T(u,v) - P)
//            = T(u,v) dot T(u,v) - T(u,v) dot P  - P dot T(u,v) + P dot P
//   and it is also commutative
//            = T(u,v) dot T(u,v) - P dot T(u,v) - P dot T(u,v) + P dot P
//            = T(u,v) dot T(u,v) - 2 * P dot T(u,v) + P dot P
//   substitute in T(u,v) = A + u(B-A) + v(C-A)
//            = (A + u(B-A) + v(C-A)) dot (A + u(B-A) + v(C-A)) - 2 * P dot (A + u(B-A) + v(C-A)) + P dot P
//   simplify notation using:
//      B = B - A
//      C = C - A
//            = (A + uB + vC) dot (A + uB + vC) - 2 * P dot (A + uB + vC) + P dot P
//            = A.A + A.uB + A.vC + uB.A + uB.uB + uB.vC + vC.A + vC.uB + vC.vC - 2*P.A - 2*P.uB - 2*P.vC + P.P
//   combine like terms and pull scalars out
//            = A.A + 2uA.B + 2vA.C + u^2B.B + 2uvB.C + v^2C.C - 2P.A - 2uP.B - 2vP.C + P.P
//   combine like terms by scalers
//            = u^2(B.B) + v^2(C.C) + 2uv(B.C) + 2u(A.B - P.B) + 2v(A.C - P.C) + (A.A - 2*P.A + P.P)
//           
//   simplify notation using:
//      a = B.B
//      b = C.C
//      c = 2*(B.C)
//      d = 2*(A.B - P.B)
//      e = 2*(A.C - P.C)
//      f = A.A - 2*P.A + P.P
// 	          = au^2 + bv^2 + cuv + du + ev + f
// 
// we can find the roots of this quadratic by finding where the gradient is zero
//   Q'(u,v) = au^2 + bv^2 + cuv + du + ev + f
// 
// I just used wolfram-alpha to solve these... type it in like this
//       grad(ax^2 + by^2 + cxy + dx + ey + f)      (wolfram alpha prefers x,y over u,v)
//       = d + 2ax + cy , e + cx + 2by
// Solve for when these are both zero, again using wolfram alpha
//       solve(d + 2ax + cy = 0, e + cx + 2by = 0)
//       
//       denom = 4ab - c^2
//       u = ec - 2bd / denom
//       v = cd - 2ea / denom
//       where denom != 0 and b != 0
//
// -------------
// Return Value:
// -------------
// will return an integer representing the region the point is in, and fill out the uv's appropriately
//             
//          |  
//      3   |
//          |
//  ------- * C
//          |\
//          | \    6
//      5   |  \
//          | 0 \
//          |    \
// ------ A *-----* B -------
//          |     |   
//     1    |  4  |    2

enum TriangleRegion
{
	TriangleRegion_ABC = 0, //  inside the triangle
	TriangleRegion_A,		//  closest point is A
	TriangleRegion_B,		//  closest point is B
	TriangleRegion_C,		//  closest point is C
	TriangleRegion_AB,		//  closest edge is AB
	TriangleRegion_AC,		//  closest edge is AC
	TriangleRegion_BC,		//  closest edge is BC
};

inline TriangleRegion ClosestPoint_TrianglePointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc, float& u, float& v)
{
	// must provide 3 different points or its not a valid triangle
	assert(va != vb);
	assert(va != vc);
	assert(vb != vc);

	vector3 A = va;
	vector3 B = vb - va;
	vector3 C = vc - va;
	float a = B.dot(B);
	float b = C.dot(C);
	float c = 2*(B.dot(C));
	float d = 2*(A.dot(B) - p.dot(B));
	float e = 2*(A.dot(C) - p.dot(C));
	float f = A.dot(A) - 2*p.dot(A) + p.dot(p);

	float denom = 4*a*b - c*c;
	assert(!FloatEquals(denom, 0.0f));
	assert(!FloatEquals(b, 0.0f));

	u = (e*c - 2*b*d) / denom;
	v = (c*d - 2*e*a) / denom;

	if (FloatLessThanEquals(u, 0.0f) && FloatLessThanEquals(v, 0.0f))
	{
		u = 0.0f;
		v = 0.0f;
		return TriangleRegion_A;
	}

	if (FloatLessThanEquals(v,0.0f) && FloatGreaterThanEquals(u,1.0f))
	{
		u = 1.0f;
		v = 0.0f;
		return TriangleRegion_B;
	}

	if (FloatLessThanEquals(u,0.0f) && FloatGreaterThanEquals(v,1.0f))
	{
		u = 0.0f;
		v = 1.0f;
		return TriangleRegion_C;
	}

	if (FloatLessThanEquals(v,0.0f) && u > 0.0f && FloatLessThanEquals(u, 1.0f))
	{
		v = 0.0f;
		return TriangleRegion_AB;
	}

	if (FloatLessThanEquals(u, 0.0f) && v > 0.0f && FloatLessThanEquals(v, 1.0f))
	{
		u = 0.0f;
		return TriangleRegion_AC;
	}

	if (u + v > 1.0f)
	{
		float denom = 1.0f / (u + v);
		u *= denom;
		v *= denom;
		return TriangleRegion_BC;
	}

	assert(u > 0.0f && v > 0.0f);
	assert(u < 1.0f && v < 1.0f);

	return TriangleRegion_ABC;
}

inline vector3 ClosestPoint_TrianglePoint(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc)
{
	float u,v;
	ClosestPoint_TrianglePointRatio(p, va, vb, vc, u,v);
	return va + (vb-va)*u + (vc-va)*v;
}



// Continuing the math shown above with 3 points:
// T(u,v,w) = A + u(B-A) + v(C-A) + w(D-A)
//  --> any point in a tetrahedron can be represented by one of the points plus some contribution of each edge
// Q(u,v,w) = | T(u,v,w) - P |^2
//  --> the distance (squared) to the origin can be represented as the magnitude of the origin to some point on the tetrahedron
// Q(u,v,w) = (P dot P) - 2(P dot T(u,v,w)) + ( T(u,v,w) dot T(u,v,w) )
//  --> dot identity
//          = (P.P - 2(P.(A + u(B-A) + v(C-A) + w(D-A))) + ((A + u(B-A) + v(C-A) + w(D-A)).(A + u(B-A) + v(C-A) + w(D-A))
//  --> rename 
//        B=B-A
//        C=C-A
//        D=D-A
//          = (P.P - 2(P.(A + uB + vC + wD)) + (A + uB + vC + wD).(A + uB + vC + wD)
//  --> expand 
//          = P.P - 2P.A - 2P.Bu - 2P.Cv - 2P.Dw + A.A + 2A.Bu + 2A.Cv + 2A.Dw + B.Bu^2 + 2B.Cuv + 2B.Duw + v^2C.C + 2C.Dvw + w^2D.D
//  --> combine like terms
// = P.P - 2P.A  + A.A 
//          = B.Bu^2 + C.Cv^2 + D.Dw^2 + 2B.Cuv + 2B.Duw + 2C.Dvw + 2(A.B - P.B)u + 2(A.C - P.C)v + 2(A.D - P.D)w + (P.P - 2P.A + A.A)
//  --> define some terms to make it more readable:
//     a = B.B
//     b = C.C
//     c = D.D
//     d = B.C
//     e = B.D
//     f = C.D
//     g = A.B - P.B
//     h = A.C - P.C
//     i = A.D - P.D
//     j = P.P - 2P.A + A.A
//         = au^2 + bv^2 + cw^2 + 2duv + 2euw + 2fvw + 2gu + 2hv + 2iw + j
// 
//	find the gradient:
//	grad(Q) = gradient(au^2 + bv^2 + cw^2 + 2duv + 2euw + 2fvw + 2gu + 2hv + 2iw + j)
//          = 2(ax + dy + ez + g), 2(by + dx + fz + h), 2(cz + ex + fy + i)
//
//	solve(au + dv + ew + g = 0, bv + du + fw + h = 0, cw + eu + fv + i = 0)
// 
//  Wolfram alpha input:
//	  solve(a*u + d*v + e*w + g = 0, b*v + d*u + f*w + h = 0, c*w + e*u + f*v + i = 0) for u,v,w
//  Wolfram alpha output:
//    u = -(b c g - b e i - c d h + d f i + e f h + f^2 (-g))/(a b c - a f^2 - b e^2 - c d^2 + 2 d e f), 
//    v = -(-a c h + a f i + c d g - d e i + e^2 h - e f g)/(-a b c + a f^2 + b e^2 + c d^2 - 2 d e f), 
//    w = -(-a b i + a f h + b e g + d^2 i - d e h - d f g)/(-a b c + a f^2 + b e^2 + c d^2 - 2 d e f)
//  
//  Or written more simply:
//    denom = abc - aff - bee - cdd + 2def;
//    u = -bcg + bei + cdh - dfi - efh + gff / denom
//    v =  ach + afi + cdg - dei - eeh - efg / denom
//    w = -abi + afh + beg + ddi + deh - dfg / denom
// 
//  I think we have the constraints:
//   denom != 0
//   be - df != 0
//   c != 0
//  Which if we hit it I think means that our inputs are not solvable (i.e. a straight line, or multiple of the same point)
//  TODO: look into above
// 
// 15 regions:
// - region A,B,C,D - it is further towards a single point than any other
//  --> return values 1,2,3,4
// - region AB,AC,AD,BC,BD,CD - it is further towards an edge than any other
//  --> return values 5,6,7,8,9,10
// - region ABC, ABD, ACD, BCD - it is further towards a triangle than any other
//  --> return values 11,12,13,14
// - region ABCD - it is within the tetrahedron
//  --> return value 0
//
enum TetrahedronRegion
{
	TetrahedronRegion_None = -1,

	TetrahedronRegion_ABCD = 0,
	TetrahedronRegion_A,
	TetrahedronRegion_B,
	TetrahedronRegion_C,
	TetrahedronRegion_D,
	TetrahedronRegion_AB,
	TetrahedronRegion_AC,
	TetrahedronRegion_AD,
	TetrahedronRegion_BC,
	TetrahedronRegion_BD,
	TetrahedronRegion_CD,
	TetrahedronRegion_ABC,
	TetrahedronRegion_ABD,
	TetrahedronRegion_ACD,
	TetrahedronRegion_BCD,
};

inline TetrahedronRegion ClosestPoint_TetrahedronPointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc, const vector3& vd, float& u, float& v, float& w)
{
	vector3 P = p;
	vector3 A = va;
	vector3 B = (vb-va);
	vector3 C = (vc-va);
	vector3 D = (vd-va);
	float a = B.dot(B);
	float b = C.dot(C);
	float c = D.dot(D);
	float d = B.dot(C);
	float e = B.dot(D);
	float f = C.dot(D);
	float g = A.dot(B) - P.dot(B);
	float h = A.dot(C) - P.dot(C);
	float i = A.dot(D) - P.dot(D);
	float j = P.dot(P) - 2*P.dot(A) + A.dot(A);
	

	float denom = (a*b*c - a*f*f - b*e*e - c*d*d + 2*d*e*f);
	if (FloatEquals(denom, 0.f))
	{
		// malformed input?  all points in a line? we definately can't continue and divide by zero
		return TetrahedronRegion_None;
	}

	// could wait to do divide until we know we need to... but easier to read this way
	u = (-b*c*g + b*e*i + c*d*h - d*f*i - e*h*f + g*f*f) / denom;
	v = (-a*c*h + a*f*i + c*d*g - d*e*i + h*e*e - e*g*f) / denom;
	w = (-a*b*i + a*h*f + b*e*g + i*d*d - d*e*h - d*g*f) / denom;
	
	// region A
	if (u <= 0.0f && v <= 0.0f && w <= 0.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 0.0f;
		return TetrahedronRegion_A;
	}
	// region B
	if (u >= 1.0f && v <= 0.0f && w <= 0.0f)
	{
		u = 1.0f;
		v = 0.0f;
		w = 0.0f;
		return TetrahedronRegion_B;
	}
	// region C
	if (u <= 0.0f && v >= 1.0f && w <= 0.0f)
	{
		u = 0.0f;
		v = 1.0f;
		w = 0.0f;
		return TetrahedronRegion_C;
	}
	// region D
	if (u <= 0.0f && v <= 0.0f && w >= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 1.0f;
		return TetrahedronRegion_D;
	}
	// region AB
	if (u >= 0.0f && u <= 1.0f && v <= 0.0f && w <= 0.0f)
	{
		v = 0.0f;
		w = 0.0f;
		return TetrahedronRegion_AB;
	}
	// region AC
	if (u <= 0.0f && v >= 0.0f && v <= 1.0f && w <= 0.0f)
	{
		u = 0.0f;
		w = 0.0f;
		return TetrahedronRegion_AC;
	}
	// region AD
	if (u <= 0.0f && v <= 0.0f && w >= 0.0f && w <= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		return TetrahedronRegion_AD;
	}

	// either region BC or ABC
	if (u >= 0.0f && v >= 0.0f && w <= 0.0f)
	{
		w = 0.0f;
		if (u + v >= 1.0f)
		{
			float divisor = u + v;
			u = u / divisor;
			v = v / divisor;
			return TetrahedronRegion_BC;
		}
		return TetrahedronRegion_ABC;
	}

	// either region BD or ABD
	if (u >= 0.0f && v <= 0.0f && w >= 0.0f)
	{
		v = 0.0f;
		if (u + w >= 1.0f)
		{
			float divisor = u + w;
			u = u / divisor;
			w = w / divisor;
			return TetrahedronRegion_BD;
		}
		return TetrahedronRegion_ABD;
	}

	// either CD or ACD
	if (u <= 0.0f && v >= 0.0f && w >= 0.0f)
	{
		u = 0.0f;
		if (v + w > 1.0f)
		{
			float divisor = v + w;
			v = v / divisor;
			w = w / divisor;
			return TetrahedronRegion_CD;
		}
		return TetrahedronRegion_ACD;
	}

	assert(u > 0.0f && v > 0.0f && w > 0.0f);

	if (u + v + w > 1.0f)
	{
		float divisor = u + v + w;
		u = u / divisor;
		v = v / divisor;
		w = w / divisor;
		return TetrahedronRegion_BCD;
	}

	assert(u < 1.0f && v < 1.0f && w < 1.0f);

	// if we got here there is a positive contribution of all edges
	// and the total sum doesn't push it outside the tetrahedron
	// therefore the point must be within the tetrahedron and the weights must
	// be correct
	return TetrahedronRegion_ABCD;
}


//******************************************************************************
inline vector3 GetDirectionToOrigin(const vector3& a)
{
	return -a;
}
//******************************************************************************
inline vector3 GetDirectionToOrigin(const vector3& a, const vector3& b)
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
inline vector3 GetDirectionToOrigin(const vector3& a, const vector3& b, const vector3& c)
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

//========================================================================
// Public APIs
//========================================================================
void Util_Test();