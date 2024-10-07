#include "test.h"

#include "test.h"

#include "platform.h"
#include "physics.h"
#include "lib.h"
#include "simplex.h"
#include "geometry.h"
#include "debug_draw.h"
#include "util.h"
#include "physics_util.h"
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
	s_simplex.verts.clear();
	s_simplex.verts.resize(1);
	s_simplex.verts[0].A = s_triangle.GetPointFurthestInDirection({1,0,0}, s_triangleTransform, false);
	s_simplex.verts[0].B = s_box.GetPointFurthestInDirection({-1,0,0}, s_boxTransform, false);
	s_simplex.verts[0].p = s_simplex.verts[0].A - s_simplex.verts[0].B;
	s_result = COLLISION_RESULT_NONE;
	s_collisionFound = false;
	s_collisionData.depth = 0.0f;
	s_collisionData.penetrationDirection = vector3();
}
//-------------------------------------------------------------------------------------------------
static void HandleInput(float dt)
{
	if (Platform_InputChangedDown('Z'))
	{
		s_diffView = !s_diffView;
		ResetSimplex();
	}

	if (Platform_InputChangedDown('T'))
	{
		switch (s_result)
		{
			case COLLISION_RESULT_NONE:
			case COLLISION_RESULT_CONTINUE:
			{
				s_result = DetectCollisionStep(s_collisionParams, false, s_simplex);
			}
			break;

			case COLLISION_RESULT_NO_OVERLAP:
			case COLLISION_RESULT_OVERLAP:
			{
				s_collisionFound = FindIntersectionPoints(s_collisionParams, s_simplex, 1, false, &s_collisionData);
			}
			break;

			default: assert(false);
		}
	}
	if (Platform_InputChangedDown('W'))
	{
		s_boxTransform.translate({ 0, 1, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown('A'))
	{
		s_boxTransform.translate({ -1, 0, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown('S'))
	{
		s_boxTransform.translate({ 0, -1, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown('D'))
	{
		s_boxTransform.translate({ 1, 0, 0 });
		ResetSimplex();
	}

	if (Platform_InputChangedDown(ARROW_KEY_UP))
	{
		s_triangleTransform.translate({ 0, 1, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_DOWN))
	{
		s_triangleTransform.translate({ 0, -1, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_LEFT))
	{
		s_triangleTransform.translate({ -1, 0, 0 });
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_RIGHT))
	{
		s_triangleTransform.translate({ 1, 0, 0 });
		ResetSimplex();
	}
}
//-------------------------------------------------------------------------------------------------
static void Update(float dt)
{
	HandleInput(dt);
}
//-------------------------------------------------------------------------------------------------
static void GetMinkowskiDifference(vector3* diff)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			vector3 t = s_triangleTransform * s_triangle.m_mesh.m_vertexPos[i];
			vector3 b = s_boxTransform * s_box.m_mesh.m_vertexPos[j];
			vector3 v = t - b;
			diff[i * 4 + j] = v;
		}
	}
}
//-------------------------------------------------------------------------------------------------
static int get_orientation(const vector3& p, const vector3& q, const vector3& r)
{
	float val = (q.y - p.y) * (r.x - q.x) -
		(q.x - p.x) * (r.y - q.y);

	if (FloatEquals(val, 0.0f))
		return 0;

	return (val > 0.0f) ? 1 : 2;
}
//-------------------------------------------------------------------------------------------------
static void DrawConvexHull(const std::vector<vector3>& diff)
{
	// This is quick and dirty, assuming 2D... eventually want to make a 3D version and generalize it
	const int numPoints = static_cast<int>(diff.size());
	
	// sort by x
	int l = 0;
	for (int i = 1; i < numPoints; i++)
		if (diff[i].x < diff[l].x)
			l = i;

	int p = l, q;
	glBegin(GL_LINE_LOOP);
	do
	{
		glVertex3fv((GLfloat*)&diff[p]);
		q = (p + 1) % numPoints;
		for (int i = 0; i < numPoints; i++)
		{
			if (get_orientation(diff[p], diff[i], diff[q]) == 2)
				q = i;
		}
		p = q;

	} while (p != l);
	glEnd();
}
//-------------------------------------------------------------------------------------------------
static void DrawArrow(vector3 startPos, vector3 endPos)
{
	vector3 v = endPos - startPos;
	vector3 arrow_tip_begin = endPos - v * 0.1f; // start the arrow tip 10% from the end
	vector3 v_normal(-v.y, v.x, v.z);
	vector3 arrow_tip_width = v_normal * 0.1f; // pitch the arrow tip out 10% of the length
	vector3 arrow_tip_edge_1 = arrow_tip_begin + arrow_tip_width;
	vector3 arrow_tip_edge_2 = arrow_tip_begin - arrow_tip_width;
	GLfloat old_line_width;
	glGetFloatv(GL_LINE_WIDTH, &old_line_width);
	glLineWidth(3.f);

	glBegin(GL_LINES);
	
	// base line
	glVertex3fv((GLfloat*)&startPos);
	glVertex3fv((GLfloat*)&endPos);
	// arrow tip
	glVertex3fv((GLfloat*)&arrow_tip_edge_1);
	glVertex3fv((GLfloat*)&endPos);
	glVertex3fv((GLfloat*)&arrow_tip_edge_2);
	glVertex3fv((GLfloat*)&endPos);
	glEnd();

	glLineWidth(old_line_width);
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


	DebugDraw_RemoveAll();

	const int textHeight = 24; // probably could make this API better to add lines or something
	int currentTextHeight = 0;
	if (s_diffView)
	{
		DebugDraw_AddString("Minkowski Difference", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += 10 + textHeight;
		DebugDraw_AddString("Z - switch views", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;
		DebugDraw_AddString("T - advance", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;

		// draw the minkowski difference w/ convex hull
		MinkowskiDifference diff;
		PhysUtil_GenerateMinkowskiDifference(s_triangle, s_triangleTransform, s_box, s_boxTransform, &diff);

		// draw each point
		glColor3f(0.0f, 1.0f, 1.0f);
		glBegin(GL_POINTS);
		for (int i = 0; i < diff.allPoints.size(); i++)
		{
			glVertex3fv((GLfloat*)&diff.allPoints[i]);
		}
		glEnd();

		DrawConvexHull(diff.allPoints);
	
		glPointSize(8.0f);

		// some feedback if we successfully found the origin
		if (s_result == COLLISION_RESULT_OVERLAP)
		{
			DebugDraw_AddString("Found Origin", 0, 10 + currentTextHeight, TEXT_COLOR_WHITE);
			currentTextHeight += textHeight;
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
			vector3 minkowski_collision_start = s_collisionData.penetrationDirection * s_collisionData.depth;
			glColor3f(0.0, 0.0f, 1.0f);
			DrawArrow(minkowski_collision_start, minkowski_collision_start + s_collisionData.penetrationDirection);
		}
	}
	else
	{
		DebugDraw_AddString("Overlapping Shapes", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += 10 + textHeight;
		DebugDraw_AddString("WASD - Move shape A", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;
		DebugDraw_AddString("Arrow Keys - Move shape B", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;
		DebugDraw_AddString("T - advance", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;
		DebugDraw_AddString("Z - switch views", 0, currentTextHeight, TEXT_COLOR_WHITE);
		currentTextHeight += textHeight;

		if (s_simplex.verts.size() > 0)
		{
			currentTextHeight += 10;
			char buf[256];
			sprintf_s(buf, "Simplex Size: %d", (int)s_simplex.verts.size());
			DebugDraw_AddString(buf, 0, currentTextHeight, TEXT_COLOR_WHITE);
			currentTextHeight += textHeight;
		}


		glColor3f(0.0, 1.0f, 0.0f);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 3; i++)
		{
			vector3 v = s_triangleTransform * s_triangle.m_mesh.m_vertexPos[i];
			glVertex3fv((GLfloat*)&v);
		}
		glEnd();

		glColor3f(0.0, 0.0f, 1.0f);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 4; i++)
		{
			vector3 v = s_boxTransform * s_box.m_mesh.m_vertexPos[i];
			glVertex3fv((GLfloat*)&v);
		}
		glEnd();

		if (s_collisionFound)
		{
			const vector3 penetration_offset = -s_collisionData.penetrationDirection * s_collisionData.depth;

			// to illustrate the movement, find the point furthest in penetration direction on the triangle
			vector3 a = s_triangle.m_mesh.m_vertexPos[0];
			vector3 b = s_triangle.m_mesh.m_vertexPos[1];
			vector3 c = s_triangle.m_mesh.m_vertexPos[2];
			vector3 arrow_start = s_triangleTransform * ClosestPoint_TrianglePoint(penetration_offset * 10.0f, a, b, c);
			DrawArrow(arrow_start, arrow_start + penetration_offset);

			// draw a shadow triangle if we moved it by penetration distance in penetration direction
			glColor3f(1.0, 1.0f, 0.0f);
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < 3; i++)
			{
				vector3 v = penetration_offset + (s_triangleTransform * s_triangle.m_mesh.m_vertexPos[i]);
				glVertex3fv((GLfloat*)&v);
			}
			glEnd();
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Test2D:
// 
// This is meant to be a visualization and test of the collision detection in 2D because it is easier
// to visually see.
// It will create two shapes that you can move with WASD keys and then you can press the T button to advance
// the GJK/EPA algorithms one step at a time.
// You can also press Z to swap between viewing the objects in object space and viewing what the algorithm
// is doing in Minkowski-Difference space.
// What you should see is:
//   - The algorithm tries to find three points in Minkowski Difference space that fully encapsulate the origin
//   - If it does, a collusion must have occured, and it then iteratively expands out from the closest point
//     in the generated simplex (originally three points) until it gets to an edge in the Minkowski difference
//   - It will then show you the penetration depth and direction
//   - In object space, it will draw a second yellow object where the collision detector determined it should
//     move the object to get out of the collision (the minimum distance possible)
//-------------------------------------------------------------------------------------------------
void Test2D()
{
	Platform_SetCameraPos({ 0, 0, 20 });
	Platform_SetCameraLookAt({ 0,0,0 });
	
	s_triangle.m_mesh.AddVertex({ 0, 1, 0 }, {0, 0, 1});
	s_triangle.m_mesh.AddVertex({ 4, 0, 0 }, {0, 0, 1});
	s_triangle.m_mesh.AddVertex({ 4, 2, 0 }, {0, 0, 1});

	s_box.m_mesh.AddVertex({ 0, 0, 0 }, { 0, 0, 1});
	s_box.m_mesh.AddVertex({ 5, 0, 0 }, { 0, 0, 1});
	s_box.m_mesh.AddVertex({ 5, 5, 0 }, { 0, 0, 1});
	s_box.m_mesh.AddVertex({ 0, 5, 0 }, { 0, 0, 1});

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