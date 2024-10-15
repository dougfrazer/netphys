#include "test.h"

#include "platform.h"
#include "physics.h"
#include "lib.h"
#include "simplex.h"
#include "physics_shape.h"
#include "debug_draw.h"
#include <vector>

#include "windows.h"
#include <gl/GL.h>
#include <gl/GLU.h>

static struct
{
	MeshPhysicsShape* m_shape = nullptr;
	Physics* m_phys = nullptr;
} s_circle;

static struct
{
	MeshPhysicsShape* m_shape = nullptr;
	Physics* m_phys = nullptr;
} s_board;

static CollisionData s_collisionData;
static Simplex s_simplex;
static vector3 s_searchDirection;
static COLLISION_RESULT s_result = COLLISION_RESULT_NONE;
static int s_iterCount = 0;
static CollisionParams s_collisionParams;
static bool s_simplexView = false;
static bool s_collisionFound = false;

static void ResetSimplex()
{
	vector3 a_local = s_collisionParams.a->GetPointFurthestInDirection({1,0,0}, matrix4(), true);
	vector3 b_local = s_collisionParams.b->GetPointFurthestInDirection({-1,0,0}, matrix4(), true);
	s_simplex.verts.resize(1);
	s_simplex.verts[0].A = s_collisionParams.aTransform * a_local;
	s_simplex.verts[0].B = s_collisionParams.bTransform * b_local;
	s_simplex.verts[0].p = s_simplex.verts[0].A - s_simplex.verts[0].B;
	s_simplex.faces.clear();
	s_simplex.m_containsOrigin = false;
	s_searchDirection = GetSearchDirection(s_simplex);
	s_result = COLLISION_RESULT_NONE;
	s_collisionFound = false;
	s_collisionData.depth = 0.0f;
	s_collisionData.penetrationDirection = vector3();
}

static void HandleInput(float dt)
{
	Platform_BasicCameraInput(dt);

	if (Platform_InputChangedDown('T'))
	{
		switch (s_result)
		{
			case COLLISION_RESULT_NONE:
			case COLLISION_RESULT_CONTINUE:
			{
				s_result = DetectCollisionStep(s_collisionParams, true, s_simplex);
				if (s_simplex.size() < 4)
				{
					s_searchDirection = GetSearchDirection(s_simplex);
				}
				if (s_simplex.m_containsOrigin)
				{
					s_simplex.SetupForEPA();
				}
			}
			break;

			case COLLISION_RESULT_NO_OVERLAP:
			case COLLISION_RESULT_OVERLAP:
			{
				s_collisionFound = FindIntersectionPoints(s_collisionParams, s_simplex, 1, true, &s_collisionData);
			}
			break;

		default: assert(false);
		}
	}

	if (Platform_InputChangedDown('R'))
	{
		ResetSimplex();
	}

	// move and rotate object
	if (Platform_InputChangedDown(ARROW_KEY_UP))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + Coordinates::GetUp());
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_DOWN))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + Coordinates::GetDown());
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_LEFT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + Coordinates::GetLeft());
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (Platform_InputChangedDown(ARROW_KEY_RIGHT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + Coordinates::GetRight());
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (Platform_InputChangedDown('Z'))
	{
		s_simplexView = !s_simplexView;
		Platform_ResetCamera();
		ResetSimplex();
	}
}

static void Update(float dt)
{
	HandleInput(dt);
}

static void DrawCoordinateAxis()
{

	// Coordinate Axis Lines
	glBegin(GL_LINES);

	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(-100.f, 0.f, 0.f);
	glVertex3f(100.f, 0.f, 0.f);

	glColor3f(0.f, 1.0, 0.f);
	glVertex3f(0.f, -100.f, 0.f);
	glVertex3f(0.f, 100.f, 0.f);

	glColor3f(0.f, 0.f, 1.0f);
	glVertex3f(0.f, 0.f, -100.f);
	glVertex3f(0.f, 0.f, 100.f);

	glEnd();
}

static void Draw()
{
	glPointSize(4.f);

	constexpr int text_height = 18;
	int textPos = 0;
	DebugDraw_RemoveAll();

	if (s_simplexView)
	{
		DebugDraw_AddString("Simplex View", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("T - advance algorithm", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("Z - switch view", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;

		glDisable(GL_LIGHTING);

		GLfloat modelMatrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glPushMatrix();
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&modelMatrix);

		DrawCoordinateAxis();

		glEnable(GL_COLOR_MATERIAL);

		const vector4 simplexColor = { 0.5f, 0.5f, 0.5f, 0.2f };
		const vector4 searchDirectionColor = { 0.9f, 0.9f, 0.9f, 0.7f };
		switch (s_simplex.size())
		{
			case 1:
			{
				DebugDraw_AddString("1 simplex", 0, textPos, TEXT_COLOR_WHITE);
				textPos += text_height;

				glBegin(GL_POINTS);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));
				glEnd();

				vector3 midPoint = (s_simplex.verts[0].p);
				vector3 searchNorm = s_searchDirection.normalize();
				vector3 searchDirEnd = midPoint + searchNorm * 5.0f;
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)&searchDirectionColor);
				glVertex3fv((GLfloat*)&midPoint);
				glVertex3fv((GLfloat*)&searchDirEnd);
				glEnd();
			}
			break;

			case 2:
			{
				DebugDraw_AddString("2 simplex", 0, textPos, TEXT_COLOR_WHITE);
				textPos += text_height;

				glBegin(GL_LINES);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));
				glVertex3fv((GLfloat*)(&s_simplex.verts[1].p));

				vector3 midPoint = (s_simplex.verts[0].p + s_simplex.verts[1].p) / 2.0f;
				vector3 searchNorm = s_searchDirection.normalize();
				vector3 searchDirEnd = midPoint + searchNorm * 5.0f;
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)&searchDirectionColor);
				glVertex3fv((GLfloat*)&midPoint);
				glVertex3fv((GLfloat*)&searchDirEnd);
				glEnd();

				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv((GLfloat*)&s_simplex.verts[0].p);
				glColor3f(0.0f, 1.0f, 0.0f);
				glVertex3fv((GLfloat*)&s_simplex.verts[1].p);
				glEnd();
			}
			break;

			case 3:
			{
				DebugDraw_AddString("3 simplex", 0, textPos, TEXT_COLOR_WHITE);
				textPos += text_height;

				glBegin(GL_TRIANGLES);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));
				glVertex3fv((GLfloat*)(&s_simplex.verts[1].p));
				glVertex3fv((GLfloat*)(&s_simplex.verts[2].p));
				glEnd();

				vector3 midPoint = (s_simplex.verts[0].p + s_simplex.verts[1].p + s_simplex.verts[2].p) / 3.0f;
				vector3 searchNorm = s_searchDirection.normalize();
				vector3 searchDirEnd = midPoint + searchNorm * 5.0f;
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)&searchDirectionColor);
				glVertex3fv((GLfloat*)&midPoint);
				glVertex3fv((GLfloat*)&searchDirEnd);
				glEnd();

				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv((GLfloat*)&s_simplex.verts[0].p);
				glColor3f(0.0f, 1.0f, 0.0f);
				glVertex3fv((GLfloat*)&s_simplex.verts[1].p);
				glColor3f(0.0f, 0.0f, 1.0f);
				glVertex3fv((GLfloat*)&s_simplex.verts[2].p);
				glEnd();
			}
			break;

			default:
			{
				char buf[128];
				sprintf_s(buf, 128, "%d simplex", (int)s_simplex.verts.size());
				DebugDraw_AddString(buf, 0, textPos, TEXT_COLOR_WHITE);
				textPos += text_height;

				if (s_simplex.faces.size() > 0)
				{
					glBegin(GL_TRIANGLES);
					float color = 0.1f;
					for (int i = 0; i < s_simplex.faces.size(); i++)
					{
						float color = fmod(0.5f + 0.05f*i, 1.0f);
						glColor4f(color, color, color, 1.0f);
						int va = s_simplex.faces[i].point_index[0];
						int vb = s_simplex.faces[i].point_index[1];
						int vc = s_simplex.faces[i].point_index[2];
						glVertex3fv((GLfloat*)(&s_simplex.verts[va].p));
						glVertex3fv((GLfloat*)(&s_simplex.verts[vb].p));
						glVertex3fv((GLfloat*)(&s_simplex.verts[vc].p));
					}
					glEnd();
				}
				else
				{
					//  ABC, ACD, DBA, BDC
					glBegin(GL_TRIANGLES);
					glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
					glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[1].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[2].p));

					glColor4f(0.55f, 0.55f, 0.55f, 1.0f);
					glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[2].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[3].p));

					glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
					glVertex3fv((GLfloat*)(&s_simplex.verts[3].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[1].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[0].p));

					glColor4f(0.65f, 0.65f, 0.65f, 1.0f);
					glVertex3fv((GLfloat*)(&s_simplex.verts[1].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[3].p));
					glVertex3fv((GLfloat*)(&s_simplex.verts[2].p));
					glEnd();
				}
			}
			break;
		}

		glPopMatrix();
	}
	else
	{
		DebugDraw_AddString("Model View", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("Arrow Keys - Move sphere", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("WASDQE - Move camera", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("LMB=pitch, RMB=roll, MMB=yaw", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("T - advance algorithm", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;
		DebugDraw_AddString("Z - switch view", 0, textPos, TEXT_COLOR_WHITE);
		textPos += text_height;

		GLfloat lightPos[][3] = {
			{ -50.f, 10.f, -50.f },
			{  50.f, 10.f, -50.f },
			{ -50.f, 10.f,  50.f },
			{  50.f, 10.f,  50.f },
		};
		GLfloat lightAmbient[][3] = {
			{ .7f, .7f, .7f },
			{ .7f, .7f, .7f },
			{ .7f, .7f, .7f },
			{ .7f, .7f, .7f },
		};
		GLfloat lightDiffuse[][3] = {
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
		};
		GLfloat lightSpecular[][3] = {
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
		};
		for (int i = 0; i < 4; i++)
		{
			glLightfv(GL_LIGHT0 + i, GL_POSITION, lightPos[i]);
			glLightfv(GL_LIGHT0 + i, GL_AMBIENT, lightAmbient[i]);
			glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lightDiffuse[i]);
			glLightfv(GL_LIGHT0 + i, GL_SPECULAR, lightSpecular[i]);
			glEnable(GL_LIGHT0 + i);
		}

		glEnable(GL_LIGHTING);
		{
			matrix4 m = s_circle.m_phys->GetTransform();
			m.transpose();
			DrawParams p;
			p.drawType = DrawType_WireFrame;
			p.color = vector4(0.0f, 0.2f, 0.0f, 0.7f);
			s_circle.m_shape->Draw(m, &p);
		}

		{
			matrix4 m = s_board.m_phys->GetTransform();
			m.transpose();
			DrawParams p;
			p.drawType = DrawType_Triangles;
			p.color = vector4(0.2f, 0.0f, 0.0f, 0.7f);
			s_board.m_shape->Draw(m, &p);
		}
	}

	switch (s_result)
	{
		case COLLISION_RESULT_CONTINUE:   DebugDraw_AddString("Continue", 0, textPos, TEXT_COLOR_WHITE);          textPos += text_height; break;
		case COLLISION_RESULT_OVERLAP:    DebugDraw_AddString("Overlap", 0, textPos, TEXT_COLOR_WHITE);           textPos += text_height; break;
		case COLLISION_RESULT_NO_OVERLAP: DebugDraw_AddString("No Overlap", 0, textPos, TEXT_COLOR_WHITE);        textPos += text_height; break;
		case COLLISION_RESULT_NONE:       DebugDraw_AddString("No Collision Data", 0, textPos, TEXT_COLOR_WHITE); textPos += text_height; break;
	}

	if (s_collisionFound)
	{
		matrix4 m = s_circle.m_phys->GetTransform();
		m.translate(s_collisionData.penetrationDirection * s_collisionData.depth);
		m.transpose();
		DrawParams p;
		p.drawType = DrawType_WireFrame;
		p.color = DrawColor_Blue;
		s_circle.m_shape->Draw(m, &p);
	}

}

static void CreateCircle()
{
	constexpr float CIRCLE_SIZE = 5.0f;
	s_circle.m_shape = new MeshPhysicsShape();
	s_circle.m_shape->CreateSphere(CIRCLE_SIZE);

	StaticPhysicsData physData;
	physData.m_gravity = { 0.0f, 0.0f, 0.0f };
	//physData.m_initialPosition = { -1.58030558f, -6.79232216f, -0.412327528f };
	//physData.m_initialRotation = { 0.359199494f, 20.3241673f, -0.861259818f };
	//physData.m_initialPosition = { 0.f, 8.72999573f, 0.f };
	//physData.m_initialRotation = { 0.f, 20.f, 0.f };
	physData.m_initialPosition = { 0.f, 3.f, 0.f };
	physData.m_initialRotation = { 0.f, 0.f, 0.f };
	physData.m_mass = 10.0f;
	physData.m_elasticity = 0.4f;
	physData.m_staticFrictionCoeff = 0.4f;
	physData.m_momentOfInertia = 0.4f * physData.m_mass * CIRCLE_SIZE * CIRCLE_SIZE;
	physData.m_inertiaTensor = matrix3(physData.m_momentOfInertia, 0.0f, 0.0f,
									   0.0f, physData.m_momentOfInertia, 0.0f,
									   0.0f, 0.0f, physData.m_momentOfInertia);
	physData.m_inverseInertiaTensor = physData.m_inertiaTensor.inv();
	physData.m_collisionResponseType = COLLISION_RESPONSE_IMPULSE;
	s_circle.m_phys = new Physics(s_circle.m_shape, physData);
}

static void CreateBoard()
{
	s_board.m_shape = new MeshPhysicsShape();
	s_board.m_shape->CreateBox(50.f,50.f,0.1f);

	StaticPhysicsData physData;
	physData.m_elasticity = 0.0f;
	physData.m_gravity = { 0.0f,0.0f,0.0f };
	physData.m_collisionResponseType = COLLISION_RESPONSE_NONE;
	physData.m_initialPosition = { 0.0f, 0.0f, 0.0f };
	physData.m_initialRotation = { 0.0f };

	s_board.m_phys = new Physics(s_board.m_shape, physData);
}

void Test3D()
{
	CreateCircle();
	CreateBoard();

	s_collisionParams.a = s_circle.m_shape;
	s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
	s_collisionParams.b = s_board.m_shape;
	s_collisionParams.bTransform = s_board.m_phys->GetTransform();
	const vector3& destination = vector3();
	s_iterCount = 0;

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