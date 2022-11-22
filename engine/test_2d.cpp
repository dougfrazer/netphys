#include "test.h"

#include "test.h"

#include "platform.h"
#include "physics.h"
#include "lib.h"
#include "simplex.h"
#include "geometry.h"
#include <vector>

#include "windows.h"
#include <gl/GL.h>
#include <gl/GLU.h>

static bool s_diffView = false;

static Geometry s_triangle;
static Geometry s_box;

static matrix4 s_triangleTransform;
static matrix4 s_boxTransform;

static CollisionParams s_collisionParams;
static Simplex s_simplex;
static COLLISION_RESULT s_result = COLLISION_RESULT_NONE;
static CollisionData s_collisionData;
static bool s_collisionFound = false;
static vector3 s_collisionDiff;
//-------------------------------------------------------------------------------------------------
void ResetSimplex()
{
	s_collisionParams.a = &s_triangle;
	s_collisionParams.b = &s_box;
	s_collisionParams.aTransform = s_triangleTransform;
	s_collisionParams.bTransform = s_boxTransform;
	s_collisionParams.aDir = {-1,0,0};
	s_collisionParams.solve3D = false;
	s_simplex.verts.clear();
	s_simplex.verts.resize(1);
	s_simplex.verts[0].A = s_triangle.Support({1,0,0}, s_triangleTransform);
	s_simplex.verts[0].B = s_box.Support({-1,0,0}, s_boxTransform);
	s_simplex.verts[0].p = s_simplex.verts[0].A - s_simplex.verts[0].B;
	s_result = COLLISION_RESULT_NONE;
	s_collisionFound = false;
}
//-------------------------------------------------------------------------------------------------
static void ProcessInput(float dt)
{
	Input input = Platform_ConsumeInput();
	//Platform_BasicCameraInput(input, dt);

	if (input.CheckKey('Z'))
	{
		s_diffView = !s_diffView;
	}

	if (input.CheckKey('T'))
	{
		if (s_result == COLLISION_RESULT_OVERLAP)
		{
			s_collisionFound = FindCollisionDepthStep(s_collisionParams, s_simplex, s_collisionData.depth, s_collisionData.pointA, s_collisionData.pointB, s_collisionDiff);
		}
		else
		{
			s_result = DetectCollisionStep(s_collisionParams, s_simplex);
		}
	}
	if (input.CheckKey('W'))
	{
		s_boxTransform.translate({ 0, 1, 0 });
		ResetSimplex();
	}
	if (input.CheckKey('A'))
	{
		s_boxTransform.translate({ -1, 0, 0 });
		ResetSimplex();
	}
	if (input.CheckKey('S'))
	{
		s_boxTransform.translate({ 0, -1, 0 });
		ResetSimplex();
	}
	if (input.CheckKey('D'))
	{
		s_boxTransform.translate({ 1, 0, 0 });
		ResetSimplex();
	}

	if (input.CheckKey(ARROW_KEY_UP))
	{
		s_triangleTransform.translate({ 0, 1, 0 });
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_DOWN))
	{
		s_triangleTransform.translate({ 0, -1, 0 });
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_LEFT))
	{
		s_triangleTransform.translate({ -1, 0, 0 });
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_RIGHT))
	{
		s_triangleTransform.translate({ 1, 0, 0 });
		ResetSimplex();
	}


}
//-------------------------------------------------------------------------------------------------
static void Update(float dt)
{
	ProcessInput(dt);
}
//-------------------------------------------------------------------------------------------------
static void GetMinkowskiDifference(vector3* diff)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			vector3 t = s_triangleTransform * s_triangle.m_mesh.m_vertices[i].pos;
			vector3 b = s_boxTransform * s_box.m_mesh.m_vertices[j].pos;
			vector3 v = t - b;
			diff[i * 4 + j] = v;
		}
	}
}
//-------------------------------------------------------------------------------------------------
static void DrawConvextHull(const vector3* diff)
{
	auto orientation = [](const vector3& p, const vector3& q, const vector3& r)
	{
		int val = (q.y - p.y) * (r.x - q.x) -
			(q.x - p.x) * (r.y - q.y);

		if (val == 0) return 0;
		return (val > 0) ? 1 : 2;
	};
	int l = 0;
	for (int i = 1; i < 12; i++)
		if (diff[i].x < diff[l].x)
			l = i;
	int p = l, q;
	glBegin(GL_LINE_LOOP);
	do
	{
		glVertex3fv((GLfloat*)&diff[p]);
		q = (p + 1) % 12;
		for (int i = 0; i < 12; i++)
		{
			if (orientation(diff[p], diff[i], diff[q]) == 2)
				q = i;
		}
		p = q;

	} while (p != l);
	glEnd();
}
//-------------------------------------------------------------------------------------------------
static void Draw()
{
	glPointSize(4.f);

	// Coordinate Axis Lines
	glBegin(GL_LINES);

	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex3f(-100.f, 0.f, 0.f);
	glVertex3f(100.f, 0.f, 0.f);
	glVertex3f(0.f, -100.f, 0.f);
	glVertex3f(0.f, 100.f, 0.f);
	glEnd();

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINT);
	glVertex3f(0,0,0);
	glEnd();

	if (s_diffView)
	{
		// draw the minkowski difference w/ convex hull
		vector3 diff[12];
		GetMinkowskiDifference(diff);
		glColor3f(0.0f, 1.0f, 1.0f);
		glBegin(GL_POINTS);
		for (int i = 0; i < 12; i++)
		{
			glVertex3fv((GLfloat*)&diff[i]);
		}
		glEnd();
		DrawConvextHull(diff);
	
		glPointSize(8.0f);

		// some feedback if we successfully found the origin... draw a dot at the origin
		glColor3f(1.0f, 1.0f, 1.0f);
		if (s_result == COLLISION_RESULT_OVERLAP)
		{
			glBegin(GL_POINTS);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glEnd();
		}

		// draw the simplex
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_POINTS);
		for (int i = 0; i < s_simplex.verts.size(); i++)
		{
			glVertex3fv((GLfloat*)&s_simplex.verts[i].p);
		}
		glEnd();
		if (s_simplex.size() > 1)
		{
			glLineWidth(3.0f);
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < s_simplex.verts.size(); i++)
			{
				glVertex3fv((GLfloat*)&s_simplex.verts[i].p);
			}
			glEnd();
			glLineWidth(1.0f);
		}

		if (s_collisionFound)
		{
			glColor3f(0.0f, 0.0f, 1.0f);
			glBegin(GL_LINES);
			glVertex3f(0.0f,0.0f,0.0f);
			glVertex3fv((GLfloat*)&s_collisionDiff);
			glEnd();
		}
	}
	else
	{
		glColor3f(0.0, 1.0f, 0.0f);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 3; i++)
		{
			vector3 v = s_triangleTransform * s_triangle.m_mesh.m_vertices[i].pos;
			glVertex3fv((GLfloat*)&v);
		}
		glEnd();

		glColor3f(0.0, 0.0f, 1.0f);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 4; i++)
		{
			vector3 v = s_boxTransform * s_box.m_mesh.m_vertices[i].pos;
			glVertex3fv((GLfloat*)&v);
		}
		glEnd();

		if (s_collisionFound)
		{
			glBegin(GL_POINTS);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3fv((GLfloat*)&s_collisionData.pointA);
			glVertex3fv((GLfloat*)&s_collisionData.pointB);
			glEnd();
		}
	}
}

//-------------------------------------------------------------------------------------------------

void Test2D()
{
	Platform_SetCameraPos({ 0, 0, 20 });
	Platform_SetCameraLookAt({ 0,0,0 });
	
	s_triangle.m_mesh.AddVertex({{ 0, 1, 0 }, {0, 0, 1}});
	s_triangle.m_mesh.AddVertex({{ 4, 0, 0 }, {0, 0, 1}});
	s_triangle.m_mesh.AddVertex({{ 4, 2, 0 }, {0, 0, 1}});

	s_box.m_mesh.AddVertex({{ 0, 0, 0 }, { 0, 0, 1}});
	s_box.m_mesh.AddVertex({{ 5, 0, 0 }, { 0, 0, 1}});
	s_box.m_mesh.AddVertex({{ 5, 5, 0 }, { 0, 0, 1}});
	s_box.m_mesh.AddVertex({{ 0, 5, 0 }, { 0, 0, 1}});

	ResetSimplex();

	PlatformParams p;
	p.windowPos[0] = 100;
	p.windowPos[1] = 100;
	p.windowSize[0] = 640;
	p.windowSize[1] = 640;
	p.updateCallback = Update;
	p.drawCallback = Draw;
	Platform_Run(p);
}