#include "test.h"
#include "util.h"

void TestUtil()
{
	{
		float x = LinePointDistance({ 0,0,0 }, { 4,11,0 }, { 6,15,0 });
		assert(x == (sqrt(5.f) * 3.f) / 5.f);
	}

	{
		vector3 a = { 0, 4, 0 };
		vector3 b = { 2, -4, 0 };
		vector3 d = { 0, 0,  0 };
		vector3 z = ClosestPoint_LinePoint(d, a, b);
		vector3 e = { 0.941176474f,  0.235294104f, 0.0f };
		assert(z == e);
	}

	// test ClosestPointTrianglePoint
	// todo: add a test for each region
	{

		vector3 p = ClosestPoint_TrianglePoint({ 0,0,0 }, { -20, -10, 2 }, { -20,10,2 }, { 20,0,2 });
		float pmag = p.magnitude();
		assert(pmag == 2.0f);
	}

	// test ClosestPointTetrahedronOrigin
	// todo: add a test for each region
	{
		vector3 a = { 0,    0, 20 };
		vector3 b = { -20, -10,  2 };
		vector3 c = { -20,  10,  2 };
		vector3 d = { 20,   0,  2 };
		float u, v, w;
		TetrahedronRegion r = ClosestPoint_TetrahedronPointRatio(vector3(), a, b, c, d, u, v, w);
		vector3 p = a + (b - a) * u + (c - a) * v + (d - a) * w;
		assert(r == TetrahedronRegion_BCD && p.magnitude() == 2.0f);
	}
	{
		vector3 a = { 0,    0, -2 };
		vector3 b = { -20, -10, -4 };
		vector3 c = { -20,  10, -4 };
		vector3 d = { 20,   0, -4 };
		float u, v, w;
		TetrahedronRegion r = ClosestPoint_TetrahedronPointRatio(vector3(), a, b, c, d, u, v, w);
		vector3 p = a + (b - a) * u + (c - a) * v + (d - a) * w;
		assert(r == TetrahedronRegion_A && p.magnitude() == 2.0f);
	}
	{
		vector3 a = { 0,    0,  2 };
		vector3 b = { -20, -10, -4 };
		vector3 c = { -20,  10, -4 };
		vector3 d = { 20,   0, -4 };
		float u, v, w;
		TetrahedronRegion r = ClosestPoint_TetrahedronPointRatio(vector3(), a, b, c, d, u, v, w);
		vector3 p = a + (b - a) * u + (c - a) * v + (d - a) * w;
		assert(r == TetrahedronRegion_ABCD && p.magnitude() == 0.0f && u == v && w > u);
	}
}