#pragma once

#include "vector.h"

static float FindClosestPointOnLineToPoint(const vector3& point, const vector3& line_start, const vector3& line_end)
{
	const vector3 ap = point - line_start;
	const vector3 ab = line_end - line_start;
	return ap.dot(ab) / ab.dot(ab);
}

static vector3 ClosestPointLinePoint(const vector3& p, const vector3& a, const vector3& b)
{
	float ratio = FindClosestPointOnLineToPoint(p, a, b);
	return a + (b - a) * ratio;
}

static float LinePointDistanceSq(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 c = ClosestPointLinePoint(p, a, b);
	return (p - c).magnitude_sq();
}

static float LinePointDistance(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 c = ClosestPointLinePoint(p, a, b);
	return (p - c).magnitude();
}

//
//  See: https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
// 
//  We can represent any point in 2D space as
//    T(u,v) = A + u(B-A) + v(C-A)
//	where A,B,C form a valid triangle
//  that is to say: any point can be represented as one point in the triangle
//  plus some scalar times each of the edges
//
//  Furthermore, we can represent the distance between a point on the triangle
//  and an arbitrary point P as:
//    Q(u,v) = | T(u,v) - P |
//  or, as the squared distance, which correlates directly to the distance:
//    Q'(u,v) = | T(u,v) - P |^2
//  which we can expand using the dot product identity
//    Q'(u,v) = (T(u,v) - P) dot (T(u,v) - P)
//            = (P dot P) - 2(P dot T(u,v)) + T(u,v) dot T(u,v)
//            = (P)dot(P) - 2(P dot (A + u(B-A) + v(C-A)) + (A + u(B-A) + v(C-A))dot(A + u(B-A) + v(C-A))
// simplify notation using:
//  B = B - A
//  C = C - A
//            = P.P - 2(P.A + uP.B + vP.C) + (A.A + 2uA.B + 2vA.C + u^2B.B + 2uvB.C + v^2C.C)
//            = P.P - 2P.A + 2uP.B + 2vP.C + (A.A + 2uA.B + 2vA.C + u^2B.B + 2uvB.C + v^2C.C)
//            = u^2B.B + v^2C.C + 2uvB.C + 2u(A.B + P.B) + 2v(P.C + A.C) + (P.P - 2P.A + A.A)
//
// simplify notation using:
//  a = B.B
//  b = C.C
//  c = B.C
//  d = A.B + P.B
//  e = A.C + P.C
//  f = (P.P - 2P.A + A.A)
//
//    Q'(u,v) = au^2 + bv^2 + 2cuv + 2du + 2ev + f
// 
// we can find the roots of this quadratic by finding where the gradient is zero
// 
// I just used wolfram-alpha to solve these... type it in like this
// 
// grad(Q') = grad(au^2 + bv^2 + 2cuv + 2du + 2ev + f) = 2(ax + cy + d), 2(by + cx + e)
// solve(ax + cy + d=0,by + cx + e=0)
//
// and we get solutions:
//   u=(ec-bd)/(ab-c^2)
//   v=(cd-ea)/(ab-c^2)
// where ab!=c^2 and b!=0 - which should be true for a well-formed triangle
//
// will return an integer representing the region the point is in, and fill out the uv's appropriately
// 
// the 'regions' represent what the point is closest to: either a vertex, an edge, or within the triangle
// regions are: A=1, B=2, C=3, AB=4, AC=5, ABC=0 (ABC means the point is within the triangle)
// it may also return if the point is ON the triangle... 6=AB 7=AC 8=AD
static int ClosestPointTrianglePointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc, float& u, float& v)
{
	vector3 A = va;
	vector3 B = vb - va;
	vector3 C = vc - va;
	float a = B.dot(B);
	float b = C.dot(C);
	float c = B.dot(C);
	float d = A.dot(B) + p.dot(B);
	float e = A.dot(C) + p.dot(C);
	float f = p.dot(p) - 2*p.dot(A) + A.dot(A);

	float denom = a*b - c*c;
	u = (e*c - b*d) / denom;
	v = (c*d - e*a) / denom;

	if (u < 0.0f && v < 0.0f)
	{
		u = 0.0f;
		v = 0.0f;
		return 1;
	}

	if (u >= 1.0f && v < 0.0f)
	{
		u = 1.0f;
		v = 0.0f;
		return 2;
	}

	if (u < 0.0f && v >= 1.0f)
	{
		u = 0.0f;
		v = 1.0f;
		return 3;
	}

	if (u > 0.0f && u < 1.0f && v < 0.0f)
	{
		v = 0.0f;
		return 4;
	}

	if (u < 0.0f && v > 0.0f && v < 1.0f)
	{
		u = 0.0f;
		return 5;
	}

	if (FloatEquals(u + v, 1.0f))
	{
		return 7;
	}

	if (u + v > 1.0f)
	{
		float divisor = u + v;
		u = u / divisor;
		v = v / divisor;
		return 5;
	}

	if (FloatEquals(v, 0.0f))
	{
		return 6;
	}

	if (FloatEquals(u, 0.0f))
	{
		return 7;
	}

	assert(u > 0.0f && v > 0.0f);
	assert(u < 1.0f && v < 1.0f);
	return 0;
}

static vector3 ClosestPointTrianglePoint(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc)
{
	float u,v;
	ClosestPointTrianglePointRatio(p, va, vb, vc, u,v);
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
// .. actual wolfram alpha input because it didnt like f,i,u,v,w:
//	solve(a*x + d*y + e*z + g = 0, b*y + d*x + m*z + h = 0, c*z + e*x + m*y + n = 0)
// 
// denom = (abc - af^2 - be^2 - cd^2 + 2def);
// u = (-bcg + bei + cdh - dfi - ehf + gf^2) / denom
// v = (-ach + afi + cdg - dei + he^2 - egf) / denom 
// w = (-abi + ahf + beg + id^2 - deh - dgf) / denom 
// 
// with constraints:
// abc - af^2 - be^2 - cd^2 + 2def != 0
// be - df != 0
// c != 0
// which should be geometrically provable for well-formed triangle?  todo: look into this
// 
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
static int ClosestPointTetrahedronPointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc, const vector3& vd, float& u, float& v, float& w)
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
	
	// could wait to do divide until we know we need to... but easier to read this way
	float denom = (a*b*c - a*f*f - b*e*e - c*d*d + 2*d*e*f);
	u = (-b*c*g + b*e*i + c*d*h - d*f*i - e*h*f + g*f*f) / denom;
	v = (-a*c*h + a*f*i + c*d*g - d*e*i + h*e*e - e*g*f) / denom;
	w = (-a*b*i + a*h*f + b*e*g + i*d*d - d*e*h - d*g*f) / denom;
	
	assert(!FloatEquals(a*b*c + 2*d*e*f , a*f*f + b*e*e + c*d*d));
	assert(!FloatEquals(b*e, d*f));
	assert(!FloatEquals(c,0.0f));

	// region A
	if (u <= 0.0f && v <= 0.0f && w <= 0.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 0.0f;
		return 1;
	}
	// region B
	if (u >= 1.0f && v <= 0.0f && w <= 0.0f)
	{
		u = 1.0f;
		v = 0.0f;
		w = 0.0f;
		return 2;
	}
	// region C
	if (u <= 0.0f && v >= 1.0f && w <= 0.0f)
	{
		u = 0.0f;
		v = 1.0f;
		w = 0.0f;
		return 3;
	}
	// region D
	if (u <= 0.0f && v <= 0.0f && w >= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 1.0f;
		return 4;
	}
	// region AB
	if (u >= 0.0f && u <= 1.0f && v <= 0.0f && w <= 0.0f)
	{
		v = 0.0f;
		w = 0.0f;
		return 5;
	}
	// region AC
	if (u <= 0.0f && v >= 0.0f && v <= 1.0f && w <= 0.0f)
	{
		u = 0.0f;
		w = 0.0f;
		return 6;
	}
	// region AD
	if (u <= 0.0f && v <= 0.0f && w >= 0.0f && w <= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		return 7;
	}

	// either region AB or ABC
	if (u >= 0.0f && v >= 0.0f && w <= 0.0f)
	{
		w = 0.0f;
		if (u + v >= 1.0f)
		{
			float divisor = u + v;
			u = u / divisor;
			v = v / divisor;
			return 8;
		}
		return 11;
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
			return 9;
		}
		return 12;
	}

	// either AC or ACD
	if (u <= 0.0f && v >= 0.0f && w >= 0.0f)
	{
		u = 0.0f;
		if (v + w > 1.0f)
		{
			float divisor = v + w;
			v = v / divisor;
			w = w / divisor;
			return 10;
		}
		return 13;
	}

	assert(u > 0.0f && v > 0.0f && w > 0.0f);

	if (u + v + w > 1.0f)
	{
		float divisor = u + v + w;
		u = u / divisor;
		v = v / divisor;
		w = w / divisor;
		return 14;
	}

	assert(u < 1.0f && v < 1.0f && w < 1.0f);

	// if we got here there is a positive contribution of all edges
	// and the total sum doesn't push it outside the tetrahedron
	// therefore the point must be within the tetrahedron and the weights must
	// be correct
	return 0;
}