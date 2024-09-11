
#include "test.h"
#include "util.h"

void Util_Test()
{
	{
		float x = LinePointDistance({ 0,0,0 }, { 4,11,0 }, { 6,15,0 });
		assert(x == (sqrt(5.f) * 3.f) / 5.f);
	}

	{
		vector3 a = { 0, 4, 0 };
		vector3 b = { 2, -4, 0 };
		vector3 d = { 0, 0,  0 };
		vector3 z = ClosestPointLinePoint(d, a, b);
		vector3 e = { 0.941176474f,  0.235294104f, 0.0f };
		assert(z == e);
	}

	// test ClosestPointTrianglePoint
	// todo: add a test for each region
	{
		
		vector3 p = ClosestPointTrianglePoint({ 0,0,0 }, { -20, -10, 2 }, { -20,10,2 }, { 20,0,2 });
		float pmag = p.magnitude();
		assert(pmag == 2.0f);
	}

	// test ClosestPointTetrahedronOrigin
	// todo: add a test for each region
	{
		vector3 a = {  0,    0, 20 };
		vector3 b = { -20, -10,  2 };
		vector3 c = { -20,  10,  2 };
		vector3 d = {  20,   0,  2 };
		float u,v,w;
		int r = ClosestPointTetrahedronPointRatio(vector3(), a,b,c,d, u, v, w);
		vector3 p = a + (b-a)*u + (c-a)*v + (d-a)*w;
		assert(r == 14 && p.magnitude() == 2.0f);
	}
	{
		vector3 a = {  0,    0, -2 };
		vector3 b = { -20, -10, -4 };
		vector3 c = { -20,  10, -4 };
		vector3 d = {  20,   0, -4 };
		float u,v,w;
		int r = ClosestPointTetrahedronPointRatio(vector3(), a,b,c,d, u, v, w);
		vector3 p = a + (b-a)*u + (c-a)*v + (d-a)*w;
		assert(r == 1 && p.magnitude() == 2.0f);
	}
		{
		vector3 a = {  0,    0,  2 };
		vector3 b = { -20, -10, -4 };
		vector3 c = { -20,  10, -4 };
		vector3 d = {  20,   0, -4 };
		float u,v,w;
		int r = ClosestPointTetrahedronPointRatio(vector3(), a,b,c,d, u, v, w);
		vector3 p = a + (b-a)*u + (c-a)*v + (d-a)*w;
		assert(r == 0 && p.magnitude() == 0.0f && u == v && w > u);
	}

}

struct CommandLineParams
{
	bool util_test = false;
	bool test_physics = false;
	bool test_3d = false;
	bool test_2d = false;
};

void ParseCommandArgs(int argc, char* argv[], CommandLineParams& outParams)
{
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "util_test") == 0)
		{
			outParams.util_test = true;
		}
		if (strcmp(argv[i], "test_physics") == 0)
		{
			outParams.test_physics = true;
		}
		if (strcmp(argv[i], "test_3d") == 0)
		{
			outParams.test_3d = true;
		}
		if (strcmp(argv[i], "test_2d") == 0)
		{
			outParams.test_2d = true;
		}
	}
}

int main(int argc, char *argv[])
{
	CommandLineParams params;
	ParseCommandArgs(argc, argv, params);

	if (params.util_test)
	{
		Util_Test();
	}
	else if (params.test_physics)
	{
		TestPhysics();
	}
	else if (params.test_3d)
	{
		Test3D();
	}
	else if (params.test_2d)
	{
		Test2D();
	}

	return 0;
}