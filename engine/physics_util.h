#pragma once

#include "physics_shape.h"

vector3 PhysUtil_GetPointFurthestInDirection(
	const std::vector<vector3>& points, 
	const vector3& direction, 
	const matrix4& world, 
	int* optional_out_index);

#ifdef TEST_PROGRAM
struct MinkowskiDifference
{
	std::vector<vector3> allPoints;
	std::vector<vector3> exteriorHull;
	vector3 center;
};

void PhysUtil_GenerateMinkowskiDifference(
	const MeshPhysicsShape& a, const matrix4& a_transform,
	const MeshPhysicsShape& b, const matrix4& b_transform,
	MinkowskiDifference* outDiff);
#endif