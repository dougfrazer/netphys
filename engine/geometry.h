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
    void AddVertex(const vertex& v);
    void AddTriangle(const vector3& a, const vector3& b, const vector3& c, const vector3& n);
    void AddTriangle(const vertex& a, const vertex& b, const vertex& c);
public: 
	std::vector<vertex>  m_vertices;
    std::vector<int>     m_indices;
    std::vector<vector3> m_normals;
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
    virtual std::vector<vector3> SupportAll(const vector3& dir, const matrix4& world) const;
    vector3 Support(const vector3& dir, const matrix4& world) const;
public:
	Mesh m_mesh;
};
//******************************************************************************
class SphereGeometry : public Geometry
{
public:
    SphereGeometry(float radius);
    virtual std::vector<vector3> SupportAll(const vector3& dir, const matrix4& world) const override;
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


