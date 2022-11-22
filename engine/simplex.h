#pragma once

#include <vector>

struct SimplexPoint
{
	vector3 p;
	vector3 A;
	vector3 B;
	float edgeWeight;
};
struct Simplex
{
	std::vector<SimplexPoint> verts;
	int size() const { return (int)verts.size(); }
};
