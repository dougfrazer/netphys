#include "geometry.h"
#include "lib.h"

//
// Based off: 
// http://www.fho-emden.de/~hoffmann/ikos27042002.pdf
//
void GeoHelpers::CreateIcosahadron(float r, Geometry* outGeo)
{
    // OK, this is just a quick-and-dirty thing to get it working
    // An icosahadron is 2 pentagrams and 2 single points, that looks kind of like a circle
    // we can use this basic shape to subdivide and return a more accurate sphere... maybe later
    static const float theta = 26.56505117707799f * PI / 180.0f;


    //
    // Vertices
    // 
    outGeo->m_vertexList = (vertex*)malloc(12 * sizeof(vertex));
    outGeo->m_numVertices = 12;
    float phi = 0;
    for (int i = 1; i < 6; i++)
    {
        outGeo->m_vertexList[i].x = r * cos(theta) * cos(phi);
        outGeo->m_vertexList[i].y = r * cos(theta) * sin(phi);
        outGeo->m_vertexList[i].z = r * -sin(theta);
        phi += 2 * PI / 5.0;
    }
    phi = 0;
    for (int i = 6; i < 11; i++)
    {
        outGeo->m_vertexList[i].x = r * cos(theta) * cos(phi);
        outGeo->m_vertexList[i].y = r * cos(theta) * sin(phi);
        outGeo->m_vertexList[i].z = r * sin(theta);
        phi += 2 * PI / 5.0;
    }

    // Do the poles
    outGeo->m_vertexList[0].x = 0.0;
    outGeo->m_vertexList[0].y = 0.0;
    outGeo->m_vertexList[0].z = -r;
    outGeo->m_vertexList[11].x = 0.0;
    outGeo->m_vertexList[11].y = 0.0;
    outGeo->m_vertexList[11].z = r;


    static const face IcosahadronFaces[] =
    {
        { 0, 2, 1, 0 },
        { 0, 3, 2, 0 },
        { 0, 4, 3, 0 },
        { 0, 5, 4, 0 },
        { 0, 1, 5, 0 },

        { 1, 2, 7, 0 },
        { 2, 3, 8, 0 },
        { 3, 4, 9, 0 },
        { 4, 5, 10, 0 },
        { 5, 1, 6, 0 },

        { 1, 7, 6, 0 },
        { 2, 8, 7, 0 },
        { 3, 9, 8, 0 },
        { 4, 10,9, 0 },
        { 5, 6, 10, 0 },

        { 6, 7, 11, 0 },
        { 7, 8, 11, 0 },
        { 8, 9, 11, 0 },
        { 9, 10,11, 0 },
        { 10,6, 11, 0 },
    };

    //
    // Faces
    // 
    outGeo->m_faceList = (face*)malloc(20 * sizeof(face));
    outGeo->m_numFaces = 20;
    memcpy(outGeo->m_faceList, &IcosahadronFaces, 20 * sizeof(face));
}
//-------------------------------------------------------------------------------------------------
void GeoHelpers::CreateCube(float width, float height, Geometry* outGeo)
{
    outGeo->m_numVertices = 8;
    outGeo->m_vertexList = (vertex*)malloc(outGeo->m_numVertices * sizeof(vertex));


    const float half_width = width / 2.0f;
    const float half_height = height / 2.0f;
    // bottom
    outGeo->m_vertexList[0].x = -half_width;
    outGeo->m_vertexList[0].y = -half_height;
    outGeo->m_vertexList[0].z = -half_width;

    outGeo->m_vertexList[1].x = half_width;
    outGeo->m_vertexList[1].y = -half_height;
    outGeo->m_vertexList[1].z = -half_width;

    outGeo->m_vertexList[2].x = half_width;
    outGeo->m_vertexList[2].y = -half_height;
    outGeo->m_vertexList[2].z = half_width;

    outGeo->m_vertexList[3].x = -half_width;
    outGeo->m_vertexList[3].y = -half_height;
    outGeo->m_vertexList[3].z = half_width;

    // top
    outGeo->m_vertexList[4].x = -half_width;
    outGeo->m_vertexList[4].y = half_height;
    outGeo->m_vertexList[4].z = -half_width;

    outGeo->m_vertexList[5].x = half_width;
    outGeo->m_vertexList[5].y = half_height;
    outGeo->m_vertexList[5].z = -half_width;

    outGeo->m_vertexList[6].x = half_width;
    outGeo->m_vertexList[6].y = half_height;
    outGeo->m_vertexList[6].z = half_width;

    outGeo->m_vertexList[7].x = -half_width;
    outGeo->m_vertexList[7].y = half_height;
    outGeo->m_vertexList[7].z = half_width;



    static constexpr face cubeFaces[] =
    {
        { 0, 1, 2, 3 },
        { 7, 4, 5, 6 },
        { 6, 5, 2, 1 },
        { 7, 0, 3, 4 },
        { 7, 6, 1, 0 },
        { 3, 2, 5, 4 },
    };
    outGeo->m_faceList = (face*)malloc(6 * sizeof(face));
    outGeo->m_numFaces = 6;
    memcpy(outGeo->m_faceList, &cubeFaces, 6 * sizeof(face));
}
