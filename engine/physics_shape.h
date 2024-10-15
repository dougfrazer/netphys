#pragma once

#include "vector.h"

#include <vector>

//******************************************************************************
// Vertex - A single point
//******************************************************************************
struct vertex
{
    vector3 pos;
    vector3 normal;
    bool operator==(const vertex& b) const { 
        return pos == b.pos && normal == b.normal;
    }
};

//******************************************************************************
// Mesh - arbitrary vertex list
//******************************************************************************
class Mesh
{
public:
    void AddVertex(const vector3& pos, const vector3& normal);
    void AddTriangle(const vector3& a, const vector3& b, const vector3& c, const vector3& n);
public: 
	std::vector<vector3> m_vertexPos;      //
    std::vector<vector3> m_vertexNormals;  // should be same size as vertexPos
    std::vector<unsigned int>     m_indices;        // index into above arrays for each point
};

//******************************************************************************
// Geometry - container class for a shape
//******************************************************************************

enum DrawType
{
    DrawType_Triangles,
    DrawType_WireFrame,
};

static const vector4 DrawColor_Red  (1.0f, 0.0f, 0.0f, 1.0f);
static const vector4 DrawColor_Green(0.0f, 1.0f, 0.0f, 1.0f);
static const vector4 DrawColor_Blue (0.0f, 0.0f, 1.0f, 1.0f);
static const vector4 DrawColor_White(1.0f, 1.0f, 1.0f, 1.0f);

struct DrawParams
{
    DrawType drawType = DrawType_Triangles;
	vector4 color = DrawColor_White;
};

class PhysicsShape
{
public:
    virtual void Draw(const class matrix4& transform, const DrawParams* params = nullptr) const {};
    // Support: Get further point in this shape in the direction specified (using the transform to world space specified)
    virtual vector3 GetPointFurthestInDirection(const vector3& dir, const matrix4& world, bool is3D) const = 0;
};
//******************************************************************************
class MeshPhysicsShape : public PhysicsShape
{
public:
	virtual void Draw(const class matrix4& transform, const DrawParams* params = nullptr) const;
	// Support: Get further point in this shape in the direction specified (using the transform to world space specified)
	virtual vector3 GetPointFurthestInDirection(const vector3& dir, const matrix4& world, bool is3D) const;

    // Temporary... eventually will use actual physics shapes describing these rather than meshes
    void CreateSphere(float radius);
    void CreateBox(float width, float depth, float height);
public:
	Mesh m_mesh;
};
//******************************************************************************
//class SpherePhysicsShape : public PhysicsShape
//{
//public:
//    SpherePhysicsShape(float radius);
//    vector3 GetPointFurthestInDirection(const vector3& dir, const matrix4& world, bool is3D) const override;
//public:
//    const float m_radius;
//};
////******************************************************************************
//class BoxPhysicsShape : public PhysicsShape
//{
//public:
//    BoxPhysicsShape(float width, float depth, float height);
//public:
//    float m_sides[3];
//};


