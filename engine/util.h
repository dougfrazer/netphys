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
struct EdgeRatios
{
	float s;
	float t;
};
static EdgeRatios ClosestPointTrianglePointRatio(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc)
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

	float s = b * e - c * d;
	float t = b * d - a * e;
	float det = a * c - b * b;

	if (s + t <= det)
	{
		if (s < 0)
		{
			if (t < 0)
			{
				// region 4
				if (d < 0)
				{
					t = 0;
					if (-d >= a)
					{
						s = 1;
					}
					else
					{
						s = -d / a;
					}
				}
				else
				{
					s = 0;
					if (e >= 0)
					{
						t = 0;
					}
					else if (-e >= c)
					{
						t = 1;
					}
					else
					{
						t = -e / c;
					}
				}
			}
			else
			{
				// region 3
				s = 0;
				if (e >= 0)
				{
					t = 0;
				}
				else if (-e >= c)
				{
					t = 1;
				}
				else
				{
					t = -e / c;
				}
			}
		}
		else if (t < 0)
		{
			// region 5
			t = 0;
			if (d >= 0)
			{
				s = 0;
			}
			else if (-d >= a)
			{
				s = 1;
			}
			else
			{
				s = -d / a;
			}
		}
		else
		{
			// region 0
			s /= det;
			t /= det;
		}
	}
	else
	{
		if (s < 0)
		{
			// region 2
			float tmp0 = b + d;
			float tmp1 = c + e;
			if (tmp1 > tmp0)
			{
				float numer = tmp1 - tmp0;
				float denom = a - 2 * b + c;
				if (numer >= denom)
				{
					s = 1;
				}
				else
				{
					s = numer / denom;
				}
				t = 1.0f - s;
			}
			else
			{
				s = 0;
				if (tmp1 <= 0)
				{
					t = 1;
				}
				else if (e >= 0)
				{
					t = 0;
				}
				else
				{
					t = -e / c;
				}
			}
		}
		else if (t < 0)
		{
			// region 6
			float tmp0 = b + e;
			float tmp1 = a + d;
			if (tmp1 > tmp0)
			{
				float numer = tmp1 - tmp0;
				float denom = a - 2 * b + c;
				if (numer >= denom)
				{
					t = 1;
				}
				else
				{
					t = numer / denom;
				}
				s = 1.0f - t;
			}
			else
			{
				t = 0;
				if (tmp1 <= 0)
				{
					s = 1;
				}
				else if (d >= 0)
				{
					s = 0;
				}
				else
				{
					s = -d / a;
				}
			}
		}
		else
		{
			// region 1
			float numer = c + e - b + d;
			if (numer <= 0)
			{
				s = 0;
			}
			else
			{
				float denom = a - 2 * b + c;
				if (numer >= denom)
				{
					s = 1;
				}
				else
				{
					s = numer / denom;
				}
			}
			t = 1.0f - s;
		}
	}

	return EdgeRatios({s,t});
}

static vector3 ClosestPointTrianglePoint(const vector3& p, const vector3& va, const vector3& vb, const vector3& vc)
{
	EdgeRatios st = ClosestPointTrianglePointRatio(p, va, vb, vc);
	return va + (vb-va)*st.s + (vc-va)*st.t;
}