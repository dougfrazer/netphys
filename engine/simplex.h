#pragma once

struct SimplexPoint
{
	int indexA;
	int indexB;
	vector3 A;
	vector3 B;
	vector3 p;
	float u;
};
struct Simplex
{
	SimplexPoint verts[4];
	int count;
	float divisor;
};
