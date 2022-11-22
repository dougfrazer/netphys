#include "geometry.h"
#include "lib.h"

#include "matrix.h"
#include <vector>
#include "windows.h"

#include <gl/GL.h>
#include <gl/GLU.h>

//-------------------------------------------------------------------------------------------------
// mesh class
//-------------------------------------------------------------------------------------------------
void Mesh::AddTriangle(const vector3& a, const vector3& b, const vector3& c, const vector3& n)
{
    AddVertex({a,n});
    AddVertex({b,n});
    AddVertex({c,n});
}
//-------------------------------------------------------------------------------------------------
void Mesh::AddTriangle(const vertex& a, const vertex& b, const vertex& c)
{
    AddVertex(a);
    AddVertex(b);
    AddVertex(c);
}
//-------------------------------------------------------------------------------------------------
void Mesh::AddVertex(const vertex& v)
{
    for (int i = 0; i < m_vertices.size(); i++)
    {
        if (m_vertices[i] == v)
        {
            m_indices.push_back(i);
            return;
        }
    }
    int index = (int)m_vertices.size();
    m_vertices.push_back(v);
    m_indices.push_back(index);
}

//-------------------------------------------------------------------------------------------------
// helper functions
//-------------------------------------------------------------------------------------------------
void CreateIcosahadron(float r, int numSubdivisions, Mesh* outMesh)
{
    static const float theta = 26.56505117707799f * PI / 180.0f;

    //
    // Vertices
    // 
    std::vector<vector3> vertexList(12);
    float phi = 0;
    for (int i = 1; i < 6; i++)
    {
		vertexList[i].x = r * cos(theta) * cos(phi);
		vertexList[i].y = r * cos(theta) * sin(phi);
		vertexList[i].z = r * -sin(theta);
        phi += 2 * PI / 5.0;
    }
    phi = 0;
    for (int i = 6; i < 11; i++)
    {
        vertexList[i].x = r * cos(theta) * cos(phi);
        vertexList[i].y = r * cos(theta) * sin(phi);
        vertexList[i].z = r * sin(theta);
        phi += 2 * PI / 5.0;
    }

    // Do the poles
    vertexList[0].x = 0.0;
    vertexList[0].y = 0.0;
    vertexList[0].z = -r;
    vertexList[11].x = 0.0;
    vertexList[11].y = 0.0;
    vertexList[11].z = r;

    struct Triangle
    {
        int x,y,z;
    };
    std::vector<Triangle> indexList(20);
	indexList[0] = { 0, 2, 1 };
	indexList[1] = { 0, 3, 2 };
	indexList[2] = { 0, 4, 3 };
	indexList[3] = { 0, 5, 4 };
	indexList[4] = { 0, 1, 5 };
	indexList[5] = { 1, 2, 7 };
	indexList[6] = { 2, 3, 8 };
	indexList[7] = { 3, 4, 9 };
	indexList[8] = { 4, 5, 10 };
	indexList[9] = { 5, 1, 6 };
	indexList[10] = { 1, 7, 6 };
	indexList[11] = { 2, 8, 7 };
	indexList[12] = { 3, 9, 8 };
	indexList[13] = { 4, 10,9 };
	indexList[14] = { 5, 6, 10 };
	indexList[15] = { 6, 7, 11 };
	indexList[16] = { 7, 8, 11 };
	indexList[17] = { 8, 9, 11 };
	indexList[18] = { 9, 10,11 };
	indexList[19] = { 10,6, 11 };

    for (int i = 1; i < numSubdivisions; i++)
    {
        std::vector<vector3> subdividedVertexList;
        std::vector<Triangle> subdividedIndexList;

        for (int j = 0; j < indexList.size(); j++)
        {
            //  Split the triangle into 4 triangles
            // 
            //     a                 a
            //     *                 *
            //    / \               / \
            //   /   \     -->   x *---* y
            //  /     \           / \ / \
            // *-------*         *---*---*    
            // b       c         b   z   c
            //
            // abc -> axy,xbz,zyx,zcy

            const vector3& a = vertexList[indexList[j].x];
            const vector3& b = vertexList[indexList[j].y];
            const vector3& c = vertexList[indexList[j].z];
            auto midPoint = [&](const vector3& a, const vector3& b)
            {
                return (a + b).normalize() * r;
            };
            vector3 x = midPoint(a,b);
            vector3 y = midPoint(a,c);
            vector3 z = midPoint(b,c);
            
            int index = (int)subdividedVertexList.size();
            subdividedVertexList.push_back(a);
            subdividedVertexList.push_back(x);
            subdividedVertexList.push_back(y);
            subdividedIndexList.push_back({ index, index + 1, index + 2 });

			subdividedVertexList.push_back(x);
			subdividedVertexList.push_back(b);
			subdividedVertexList.push_back(z);
            subdividedIndexList.push_back({ index + 3, index + 4, index + 5 });

			subdividedVertexList.push_back(z);
			subdividedVertexList.push_back(y);
			subdividedVertexList.push_back(x);
            subdividedIndexList.push_back({ index + 6, index + 7, index + 8 });

			subdividedVertexList.push_back(z);
			subdividedVertexList.push_back(c);
			subdividedVertexList.push_back(y);
            subdividedIndexList.push_back({ index + 9, index + 10, index + 11 });
        }

        vertexList = std::move(subdividedVertexList);
        indexList = std::move(subdividedIndexList);
    }

    for (int i = 0; i < vertexList.size(); i++)
    {
        vertex v;
        v.pos = vertexList[i];
        v.normal = vertexList[i];
        outMesh->AddVertex(v);
    }
}
//-------------------------------------------------------------------------------------------------
static int GetGLDrawFromDrawType(DrawType t)
{
    switch (t)
    {
        case DrawType_Triangles: return GL_TRIANGLES;
        case DrawType_WireFrame: return GL_LINES;
    }
    return GL_LINE_STRIP;
}
//-------------------------------------------------------------------------------------------------
void Geometry::Draw(const matrix4& t, const DrawParams* params) const
{
	GLfloat modelMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);

	glPushMatrix();

	glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);

	matrix4 transform = t * matrix4((float*)modelMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((GLfloat*)&transform);

	if (params)
	{
		glBegin(GetGLDrawFromDrawType(params->drawType));
		glColor4fv((GLfloat*)&params->color);
	}
	else
	{
		glBegin(GL_TRIANGLES);
		glColor3f(0.0, 1.0, 0.0);
	}

	for (int i = 0; i < m_mesh.m_indices.size(); i++)
	{
		glVertex3fv((GLfloat*)&m_mesh.m_vertices[m_mesh.m_indices[i]].pos);
		glNormal3fv((GLfloat*)&m_mesh.m_vertices[m_mesh.m_indices[i]].normal);
	}
	glEnd();

	glPopMatrix();
}
//-------------------------------------------------------------------------------------------------
vector3 Geometry::Support(const vector3& dir, const matrix4& world) const
{
	// get the position furthest in the direction of geo (modified by world)
	float dot_product = -std::numeric_limits<float>::infinity();
	vector3 closestPoint;
	// todo: do we need to consider all vertices, or can we iteratively solve for the furthest?
    //       possibly we can be given a previous vertex and consider all the edges to that vertex and
    //       return the connected vertex that is furthest in the direction... so we only consider
    //       a few vertices rather than the entire mesh
	for (int i = 0; i < m_mesh.m_vertices.size(); i++)
	{
		vector3 t = world * m_mesh.m_vertices[i].pos;

		float c = dir.dot(t);
		if (c >= dot_product)
		{
			dot_product = c;
			closestPoint = t;
		}
	}
	assert(dot_product != -std::numeric_limits<float>::infinity());
	return closestPoint;
}
//-------------------------------------------------------------------------------------------------
SphereGeometry::SphereGeometry(float radius)
    : m_radius(radius)
{
	CreateIcosahadron(radius, 3, &m_mesh);
}
//-------------------------------------------------------------------------------------------------
vector3 SphereGeometry::Support(const vector3& dir, const matrix4& world) const
{
	vector3 dir_normalized = dir.normalize();
	vector3 circle_position = dir_normalized * m_radius;
	return world * circle_position;
}
//-------------------------------------------------------------------------------------------------
BoxGeometry::BoxGeometry(float width, float depth, float height)
{
    m_sides[0] = width;
    m_sides[1] = depth;
    m_sides[2] = height;

    const float half_width = width / 2.0f;
    const float half_depth = width / 2.0f;
    const float half_height = height / 2.0f;

    std::vector<vector3> corners(8);
    corners[0] = {  half_width, -half_height, -half_depth };
    corners[1] = {  half_width, -half_height,  half_depth };
    corners[2] = {  half_width,  half_height, -half_depth };
    corners[3] = {  half_width,  half_height,  half_depth };
    corners[4] = { -half_width, -half_height, -half_depth };
    corners[5] = { -half_width, -half_height,  half_depth };
    corners[6] = { -half_width,  half_height, -half_depth };
    corners[7] = { -half_width,  half_height,  half_depth };

    //         
    //        6 *----------------------* 7                          
    //         /|                     /|
    //        / |                    / |
    //       /  | 4                 /  |
    //      /   *------------------/---* 5
    //   2 *----------------------* 3 /   
    //     |  /                   |  /
    //     | /                    | /
    //     |/                     |/
    //   0 *----------------------* 1       
    // 
    //    front        right      back        left         top       bottom
    //   2     3     3     7     7     6     6     2     6     7    0     1
    //   *-----*     *-----*     *-----*     *-----*     *-----*    *-----*
    //   |     |     |     |     |     |     |     |     |     |    |     |
    //   |     |     |     |     |     |     |     |     |     |    |     |
    //   *-----*     *-----*     *-----*     *-----*     *-----*    *-----*
    //   0     1     1     5     5     4     4     0     2     3    4     5
    
    // front
    m_mesh.AddTriangle(corners[0], corners[1], corners[3], Coordinates::GetOut());
    m_mesh.AddTriangle(corners[0], corners[3], corners[2], Coordinates::GetOut());
    // right
	m_mesh.AddTriangle(corners[1], corners[5], corners[7], Coordinates::GetRight());
	m_mesh.AddTriangle(corners[1], corners[7], corners[3], Coordinates::GetRight());
    // back
	m_mesh.AddTriangle(corners[5], corners[4], corners[6], Coordinates::GetIn());
	m_mesh.AddTriangle(corners[5], corners[6], corners[7], Coordinates::GetIn());
    // left
	m_mesh.AddTriangle(corners[4], corners[0], corners[2], Coordinates::GetLeft());
	m_mesh.AddTriangle(corners[4], corners[2], corners[6], Coordinates::GetLeft());
    // top
	m_mesh.AddTriangle(corners[2], corners[3], corners[7], Coordinates::GetUp());
	m_mesh.AddTriangle(corners[2], corners[7], corners[6], Coordinates::GetUp());
    // bottom
	m_mesh.AddTriangle(corners[4], corners[5], corners[1], Coordinates::GetDown());
	m_mesh.AddTriangle(corners[4], corners[1], corners[0], Coordinates::GetDown());
}