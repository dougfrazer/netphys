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

// Used for EPA
struct SimplexFace
{
	int point_index[3];
	vector3 normal;
};

struct Simplex
{
	bool m_containsOrigin = false;
	std::vector<SimplexPoint> verts;

	// BEGIN: EPA stuff
	// might be cleaner to make a separate "polytope" structure rather than shoving this in here but this is faster for now
	void SetupForEPA()
	{
		assert(verts.size() == 4);
		AddFace(0,1,2);
		AddFace(0,2,3);
		AddFace(3,1,0);
		AddFace(1,3,2);
	}
	void AddFace(int va, int vb, int vc)
	{
		int face_index = (int)faces.size();
		faces.resize(face_index + 1);

		faces[face_index].point_index[0] = va;
		faces[face_index].point_index[1] = vb;
		faces[face_index].point_index[2] = vc;

		const vector3& a = verts[va].p;
		const vector3& b = verts[vb].p;
		const vector3& c = verts[vc].p;
		const vector3 ab = (b - a);
		const vector3 ac = (c - a);
		const vector3 n = (ab.cross(ac)).normalize();
		faces[face_index].normal = n.dot(a) ? n : -n;
	}
	std::vector< SimplexFace> faces; 
	// END: EPA stuff

	int size() const { return (int)verts.size(); }

	int insert(const SimplexPoint& point, int index = -1)
	{
		int numVerts = (int)verts.size();
		verts.resize(numVerts + 1);
		if (index == -1)
		{
			verts[numVerts] = point;
			return numVerts;
		}

		for (int i = numVerts; i > index; --i)
		{
			verts[i] = verts[i - 1];
		}
		verts[index] = point;
		return index;
	}
};
