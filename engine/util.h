#pragma once

#include "vector.h"

static float ClosestPointLinePointRatio(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 ap = p - a;
	vector3 ab = b - a;
	return ap.dot(ab) / ab.dot(ab);
}

static vector3 ClosestPointLinePoint(const vector3& p, const vector3& a, const vector3& b)
{
	float ratio = ClosestPointLinePointRatio(p, a, b);
	return a + (b - a) * ratio;
}

static float LinePointDistance(const vector3& p, const vector3& a, const vector3& b)
{
	vector3 c = ClosestPointLinePoint(p, a, b);
	return (p - c).magnitude();
}

//
//  See: https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
// 
//  Using the triangle equation: T(s,t) = B + s*e0 + t*e1
//  whereas any point on the triangle is the base vector plus
//  some ratio of each of the edges (e0 and e1)
// 
// consider the regions around or within a triangle, represented by 
//  the below labels
//
//         \r2|
//          \ |
//           \|
//            \
//		r3	  |\
//			  | \  r1
//			  |  \ 
//			  | r0\
// -----------|----\----------------------
//		r4	  | r5  \  r6
//			  |      \
// we can need to compute values of s and t to satisfy the triangle equation
static int ClosestPointTrianglePointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc, float& s, float& t)
{
	vector3 vd = va - p;
	vector3 e0 = vb - va;
	vector3 e1 = vc - va;
	float a = e0.dot(e0);
	float b = e0.dot(e1);
	float c = e1.dot(e1);
	float d = e0.dot(vd);
	float e = e1.dot(vd);
	float f = vd.dot(vd);

	s = b * e - c * d;
	t = b * d - a * e;
	float det = a * c - b * b;

	if (s + t <= det)
	{
		if (s < 0.0f)
		{
			if (t < 0.0f)
			{
				if (d < 0.0f)
				{
					t = 0.0f;
					if (-d >= a)
					{
						s = 1.0f;
						return 6;
					}
					s = -d / a;
					return 5;
				}
				else
				{
					s = 0.0f;
					if (e >= 0.0f)
					{
						t = 0.0f;
						return 4;
					}
					if (-e >= c)
					{
						t = 1.0f;
						return 2;
					}

					t = -e / c;
					return 3;
				}
			}
			else
			{
				s = 0.0f;
				if (e >= 0.0f)
				{
					t = 0.0f;
					return 4;
				}

				if (-e >= c)
				{
					t = 1.0f;
					return 2;
				}

				t = -e / c;
				return 3;
			}
		}
		else if (t < 0.0f)
		{
			// region 5
			t = 0.0f;
			if (d >= 0.0f)
			{
				s = 0.0f;
				return 4;
			}

			if (-d >= a)
			{
				s = 1.0f;
				return 6;
			}

			s = -d / a;
			return 5;
		}
		else
		{
			// region 0
			s /= det;
			t /= det;
			return 0;
		}
	}
	else
	{
		if (s < 0.0f)
		{
			float tmp0 = b + d;
			float tmp1 = c + e;
			if (tmp1 > tmp0)
			{
				float numer = tmp1 - tmp0;
				float denom = a - 2 * b + c;
				if (numer >= denom)
				{
					s = 1.0f;
					t = 0.0f;
					return 6;
				}

				s = numer / denom;
				t = 1.0f - s;
				return 1;
			}
			else
			{
				s = 0.0f;
				if (tmp1 <= 0)
				{
					t = 1;
					return 2;
				}
				
				if (e >= 0)
				{
					t = 0;
					return 4;
				}

				t = -e / c;
				return 3;
			}
		}
		else if (t < 0)
		{
			float tmp0 = b + e;
			float tmp1 = a + d;
			if (tmp1 > tmp0)
			{
				float numer = tmp1 - tmp0;
				float denom = a - 2 * b + c;
				if (numer >= denom)
				{
					t = 1.0f;
					s = 0.0f;
					return 2;
				}

				t = numer / denom;
				s = 1.0f - t;
				return 1;
			}
			else
			{
				t = 0.0f;
				if (tmp1 <= 0.0f)
				{
					s = 1.0f;
					return 6;
				}

				if (d >= 0.0f)
				{
					s = 0.0f;
					return 4;
				}

				s = -d / a;
				return 5;
			}
		}
		else
		{
			float numer = c + e - b + d;
			if (numer <= 0.0f)
			{
				s = 0.0f;
				t = 1.0f;
				return 2;
			}

			float denom = a - 2 * b + c;
			if (numer >= denom)
			{
				s = 1.0f;
				t = 0.0f;
				return 6;
			}

			s = numer / denom;
			t = 1.0f - s;
			return 1;
		}
	}

	assert(false);
	return -1;
}

static vector3 ClosestPointTrianglePoint(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc)
{
	float s,t;
	ClosestPointTrianglePointRatio(p, va, vb, vc, s, t);
	return va + (vb-va)*s + (vc-va)*t;
}



// Continuing the math shown above with 3 points:
// T(u,v,w) = A + u(B-A) + v(C-A) + w(D-A)
//  --> any point in a tetrahedron can be represented by one of the points plus some contribution of each edge
// Q(u,v,w) = | T(u,v,w) |^2
//  --> the distance (squared) to the origin can be represented as the magnitude of the origin to some point on the tetrahedron
// Q(u,v,w) = ( T(u,v,w) dot T(u,v,w) )
//  --> dot identity
//  = ((A + u(B-A) + v(C-A) + w(D-A)) dot (A + u(B-A) + v(C-A) + w(D-A))
//  --> expand 
//  = (A)dot(A) + 2u(B-A)dot(A) + u^2(B-A)dot(B-A) + 2uv(B-A)dot(C-A) + 2uw(B-A)dot(D-A) + 2v(C-A)dot(A) + v^2(C-A)dot(C-A) + 2vw(C-A)dot(D-A) + 2w(D-A)dot(A) + w^2(D-A)dot(D-A)
//  --> expand further
//
// define some terms to make it more readable:
// AB = (B-A)
// AC = (C-A)
// AD = (D-A)
// a = (AB)dot(A)
// b = (AB)dot(AB)
// c = (AB)dot(AC)
// d = (AB)dot(AD)
// e = (AC)dot(A)
// f = (AC)dot(AC)
// g = (AC)dot(AD)
// h = (AD)dot(A)
// i = (AD)dot(AD)
// j = (A)dot(A)
//
// Q(u,v,w) = j + (2a)u + (b)u^2 + (2c)uv + (2d)uw + (2e)v + (f)v^2 + (2g)vw + (2h)w + (i)w^2
// we want to find points when the gradient is zero
// grad(Q) = 2(a + bu + cv + dw) , 2(e + cu + fv + gw) , 2(h + du + gv + iw)
// 
// throw the system of equations into wolfram alpha....
// a + bu + cv + dw = 0
// e + cu + fv + gw = 0
// h + du + gv + iw = 0
//
// denom = bg^2 - bfi - c^2i - 2cdg + d^2f
// u = (-ag^2 + afi - cei + cgh + deg - dhf) / denom
// v = (-aci + adg + bei - bgh + cdh - d^2e) / denom
// w = (acg - adf - beg + bhf - c^2h + cde) / denom
//
// (where bg^2 + c^2i + d^2f - (2cdg + bif) != 0 and cg - df != 0 and d != 0)
// todo: geometrically prove those are true?

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

// todo: this can be expanded to an arbitrary point to a tetrahedron, but the math is more complicated and not necessary right now

static int ClosestPointTetrahedronOriginRatio(const vector3& va, const vector3& vb, const vector3& vc, const vector3& vd, float& u, float& v, float& w)
{
	vector3 AB = (vb-va);
	vector3 AC = (vc-va);
	vector3 AD = (vd-va);
	float a = AB.dot(va);
	float b = AB.dot(AB);
	float c = AB.dot(AC);
	float d = AB.dot(AD);
	float e = AC.dot(va);
	float f = AC.dot(AC);
	float g = AC.dot(AD);
	float h = AD.dot(va);
	float i = AD.dot(AD);
	float j = va.dot(va);
	
	// could wait to do divide until we know we need to... but easier to read this way
	float denom = (-b*f*i + b*g*g + c*c*i - 2*c*d*g + d*d*f);
	u = (a*f*i - a*g*g - c*e*i + c*g*h + d*e*g - d*f*h) / denom;
	v = (-a*c*i + a*d*g + b*e*i - b*g*h + c*d*h - d*d*e) / denom;
	w = (a*c*g - a*d*f - b*e*g + b*f*h - c*c*h + c*d*e) / denom; 
	
	assert(!FloatEquals(b*g*g + c*c*i + d*d*f, b*f*i + 2*c*d*g));
	assert(!FloatEquals(d*f, c*g));
	assert(!FloatEquals(d,0.0f));

	// region A
	if (u < 0.0f && v < 0.0f && w < 0.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 0.0f;
		return 1;
	}
	// region B
	if (u >= 1.0f && v < 0.0f && w < 0.0f)
	{
		u = 1.0f;
		v = 0.0f;
		w = 0.0f;
		return 2;
	}
	// region C
	if (u < 0.0f && v >= 1.0f && w < 0.0f)
	{
		u = 0.0f;
		v = 1.0f;
		w = 0.0f;
		return 3;
	}
	// region D
	if (u < 0.0f && v < 0.0f && w >= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		w = 1.0f;
		return 4;
	}
	// region AB
	if (u > 0.0f && u <= 1.0f && v < 0.0f && w < 0.0f)
	{
		v = 0.0f;
		w = 0.0f;
		return 5;
	}
	// region AC
	if (u < 0.0f && v > 0.0f && v <= 1.0f && w < 0.0f)
	{
		u = 0.0f;
		w = 0.0f;
		return 6;
	}
	// region AD
	if (u < 0.0f && v < 0.0f && w > 0.0f && w <= 1.0f)
	{
		u = 0.0f;
		v = 0.0f;
		return 7;
	}

	// either region AB or ABC
	if (u > 0.0f && v > 0.0f && w < 0.0f)
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
	if (u > 0.0f && v < 0.0f && w > 0.0f)
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
	if (u < 0.0f && v > 0.0f && w > 0.0f)
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

	// if we got here there is a positive contribution of all edges
	// and the total sum doesn't push it outside the tetrahedron
	// therefore the point must be within the tetrahedron and the weights must
	// be correct
	return 0;
}