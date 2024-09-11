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
    //void AddTriangle(const vertex& a, const vertex& b, const vertex& c);
public: 
	std::vector<vector3> m_vertexPos;      //
    std::vector<vector3> m_vertexNormals;  // should be same size as vertexPos
    std::vector<int>     m_indices;        // index into above arrays for each point
};

//******************************************************************************
// Geometry - container class for a shape
//******************************************************************************

enum DrawType
{
    DrawType_Triangles,
    DrawType_WireFrame,
};
struct DrawParams
{
    DrawType drawType = DrawType_Triangles;
	vector4 color = {1.0f,1.0f,1.0f,1.0f};
};

class Geometry
{
public:
    virtual void Draw(const class matrix4& transform, const DrawParams* params = nullptr) const;
    // Support: Get further point in this shape in the direction specified (using the transform to world space specified)
    vector3 GetPointFurthestInDirection(const vector3& dir, const matrix4& world, int *optional_out_index = nullptr) const;

    virtual std::vector<vector3> GetAllPointsInDirection(const vector3& dir, const matrix4& world) const;
public:
	Mesh m_mesh;
};
//******************************************************************************
class SphereGeometry : public Geometry
{
public:
    SphereGeometry(float radius);
    virtual std::vector<vector3> GetAllPointsInDirection(const vector3& dir, const matrix4& world) const override;
public:
    const float m_radius;
};
//******************************************************************************
class BoxGeometry : public Geometry
{
public:
    BoxGeometry(float width, float depth, float height);
public:
    float m_sides[3];
};


