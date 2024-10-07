#pragma once

#include <vector>

struct SimplexPoint
{
	vector3 p;
	vector3 A;
	//int a_index;
	vector3 B;
	//int b_index;
	//float edgeWeight;
};
struct Simplex
{
	bool m_containsOrigin = false;
	std::vector<SimplexPoint> verts;
	int size() const { return (int)verts.size(); }

	void insert(const SimplexPoint& point, int index)
	{
		int numVerts = (int)verts.size();
		verts.resize(numVerts + 1);
		if (index == 0)
		{
			verts[numVerts] = point;
		}
		else
		{
			for (int i = numVerts; i > index; --i)
			{
				verts[i] = verts[i - 1];
			}
			verts[index] = point;
		}
	}
};
