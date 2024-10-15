#include "physics_util.h"
#include "matrix.h"
#include <algorithm>

vector3 PhysUtil_GetPointFurthestInDirection(
	const std::vector<vector3>& points,
	const vector3& direction,
	const matrix4& world,
	int* optional_out_index)
{
	// get the position furthest in the direction of geo (modified by world)
	vector3 result;
	float best_dot_product = -std::numeric_limits<float>::infinity();

	for (int i = 0; i < points.size(); i++)
	{
		vector3 world_pos = world * points[i];
		float dot_product = direction.dot(world_pos);
		// what to do if two vertices are equally in the specified direction?
		// i.e. dot_product == best_dot_product?
		if (dot_product > best_dot_product)
		{
			best_dot_product = dot_product;
			result = world_pos;
			if (optional_out_index)
			{
				*optional_out_index = i;
			}
		}
	}

	assert(best_dot_product != -std::numeric_limits<float>::infinity());

	return result;
}

static int GetPointFurthestInNormal(
	const std::vector<vector3>& points,
	const vector3& center,
	const vector3& a, const vector3& b, const vector3& c)
{
	const vector3 ab = (b-a);
	const vector3 ac = (c-a);
	vector3 n = ab.cross(ac);
	const vector3 a_center = (center - a);
	if (a_center.dot(n) > 0.0f)
	{
		n = -n;
	}

	int bestIndex = -1;
	float bestDot = 0.0f;
	for (int i = 0; i < points.size(); i++)
	{
		const vector3 ap = points[i] - a;
		float dot_product = ap.dot(n);
		if (dot_product > bestDot)
		{
			bestIndex = i;
		}
	}

	return bestIndex;
}

void PhysUtil_GenerateMinkowskiDifference(
	const MeshPhysicsShape& a_shape, const matrix4& a_transform,
	const MeshPhysicsShape& b_shape, const matrix4& b_transform,
	MinkowskiDifference* outDiff)
{
	const int numPointsInDiff = (int)a_shape.m_mesh.m_vertexPos.size() * (int)b_shape.m_mesh.m_vertexPos.size();
	outDiff->allPoints.resize(numPointsInDiff);

	for (int i = 0; i < a_shape.m_mesh.m_vertexPos.size(); i++)
	{
		for (int j = 0; j < b_shape.m_mesh.m_vertexPos.size(); j++)
		{
			vector3 t = a_transform * a_shape.m_mesh.m_vertexPos[i];
			vector3 b = b_transform * b_shape.m_mesh.m_vertexPos[j];
			vector3 v = t - b;
			outDiff->allPoints[i * 4 + j] = v;
			outDiff->center = outDiff->center + v;
		}
	}
	outDiff->center = outDiff->center / numPointsInDiff;

	
	// TODO: some sort of exterior hull algorithm
	// 
	//std::vector<vector3> hullPoints = outDiff->allPoints;
	//
	//std::sort(hullPoints.begin(), hullPoints.end(), [](const vector3& a, const vector3& b) { return a.x < b.x; });
	//
	//std::vector<int> exteriorPoints;
	//exteriorPoints.resize(6);
	//// order here matters, trying to make sure they are ccw
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetLeft(), matrix4(), &exteriorPoints[0]);
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetUp(), matrix4(), &exteriorPoints[1]);
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetIn(), matrix4(), &exteriorPoints[2]);
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetRight(), matrix4(), &exteriorPoints[3]);
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetDown(), matrix4(), &exteriorPoints[4]);
	//PhysUtil_GetPointFurthestInDirection(outDiff->allPoints, Coordinates::GetOut(), matrix4(), &exteriorPoints[5]);
	//
	//for (int i = 0; i < exteriorPoints.size() - 2; i++)
	//{
	//	const vector3& a = outDiff->allPoints[exteriorPoints[i]];
	//	const vector3& b = outDiff->allPoints[exteriorPoints[i+1]];
	//	const vector3& c = outDiff->allPoints[exteriorPoints[i+2]];
	//	int new_index = GetPointFurthestInNormal(outDiff->allPoints, outDiff->center, a, b, c);
	//	if (new_index != -1)
	//	{
	//		vector3 a_center = outDiff->center - a;
	//		vector3 a_new = outDiff->allPoints[new_index] - a;
	//		vector3 ab = b - a;
	//		if (a_new.cross(ab).dot(a_center) > 0.0f)
	//		{
	//			exteriorPoints.insert(i + 1, new_index);
	//		}
	//	}
	//}
	
}

//
//
//   found this algorithm on the internet... TODO read it 
//

//void hull(Point * list, int n, Point * *A, Point * *B) 
//{
//	Point * u, * v, * mid;
//	double t[6], oldt, newt;
//	int i, j, k, l, minl;
//
//	if (n == 1) { A[0] = list->prev = list->next = NIL; return; }
//
//	for (u = list, i = 0; i < n / 2 - 1; u = u->next, i++);
//
//	mid = v = u->next;
//	hull(list, n / 2, B, A); // recurse on left and right sides
//	hull(mid, n - n / 2, B + n / 2 * 2, A + n / 2 * 2);
//	
//	for (; ; ) // find initial bridge
//	{
//		if (turn(u, v, v->next) < 0) v = v->next;
//		else if (turn(u->prev, u, v) < 0) u = u->prev;
//		else break;
//	}
//		
//	// merge by tracking bridge uv over time
//	for (i = k = 0, j = n / 2 * 2, oldt = -INF; ; oldt = newt) 
//	{
//		t[0] = time(B[i]->prev, B[i], B[i]->next);
//		t[1] = time(B[j]->prev, B[j], B[j]->next);
//		t[2] = time(u, u->next, v);
//		t[3] = time(u->prev, u, v);
//		t[4] = time(u, v->prev, v);
//		t[5] = time(u, v, v->next);
//		for (newt = INF, l = 0; l < 6; l++)
//			if (t[l] > oldt && t[l] < newt) { minl = l; newt = t[l]; }
//		if (newt == INF) break;
//		switch (minl)
//		{
//			case 0: if (B[i]->x < u->x) A[k++] = B[i]; B[i++]->act(); break;
//			case 1: if (B[j]->x > v->x) A[k++] = B[j]; B[j++]->act(); break;
//			case 2: A[k++] = u = u->next; break;
//			case 3: A[k++] = u; u = u->prev; break;
//			case 4: A[k++] = v = v->prev; break;
//			case 5: A[k++] = v; v = v->next; break;
//		}
//	}
//	A[k] = NIL;
//
//	u->next = v; v->prev = u; // now go back in time to update pointers
//
//	for (k--; k >= 0; k--)
//		if (A[k]->x <= u->x || A[k]->x >= v->x) {
//		A[k]->act();
//		if (A[k] == u) u = u->prev; else if (A[k] == v) v = v->next;
//	}
//	else {
//		u->next = A[k]; A[k]->prev = u; v->prev = A[k]; A[k]->next = v;
//		if (A[k]->x < mid->x) u = A[k]; else v = A[k];
//	}
//}
