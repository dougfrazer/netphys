#pragma once

//******************************************************************************
// Vertex - A single point
//******************************************************************************
typedef struct
{
    float x;
    float y;
    float z;
} vertex;

//******************************************************************************
// Face - A series of vertices
//******************************************************************************
typedef struct
{
    int v1;
    int v2;
    int v3;
    int v4;
} face;


class Geometry
{
public:
    int       m_numVertices = 0;
    vertex*   m_vertexList = nullptr;

    int       m_numFaces = 0;
    face*     m_faceList = nullptr;
};

namespace GeoHelpers
{
    void CreateIcosahadron(float r, Geometry* outGeo);
    void CreateCube(float width, float height, Geometry* outGeo);
};
