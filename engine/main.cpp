
#include "test.h"
#include "util.h"

void Util_Test()
{
	float x = LinePointDistance({ 0,0,0 }, { 4,11,0 }, { 6,15,0 });
	assert(x == (sqrt(5.f) * 3.f) / 5.f);

	vector3 a = { 0, 4, 0 };
	vector3 b = { 2, -4, 0 };
	vector3 d = { 0, 0,  0 };
	vector3 z = ClosestPointLinePoint(d, a, b);
	vector3 e = { 0.941176474f,  0.235294104f, 0.0f };
	assert(z == e);

	// todo: add a test for each region
	vector3 p = ClosestPointTrianglePoint({0,0,0}, {-20, -10, 2}, {-20,10,2}, {20,0,2});
	float pmag = p.magnitude();
	assert(pmag == 2.0f);
}

int main()
{
	Util_Test();
	TestPhysics();
	//TestSimplex();
	return 0;
}