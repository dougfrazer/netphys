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
        return FloatEquals(pos.x, b.pos.x) && 
               FloatEquals(pos.y, b.pos.y) && 
               FloatEquals(pos.z, b.pos.z); 
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
    virtual vector3 GetRandomPointOnEdge() const { return m_mesh.m_vertices[0].pos; }
    // Support: Get further point in this shape in the direction specified (using the transform to world space specified)
    virtual vector3 Support(const vector3& dir, const matrix4& world) const;
public:
	Mesh m_mesh;
};
//******************************************************************************
class SphereGeometry : public Geometry
{
public:
    SphereGeometry(float radius);
    virtual vector3 GetRandomPointOnEdge() const override { return {0.0f,0.0f,m_radius}; }
    virtual vector3 Support(const vector3& dir, const matrix4& world) const override;
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


