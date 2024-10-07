
#include "test.h"
#include "util.h"

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
		TestUtil();
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