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

static struct
{
	SphereGeometry* m_geo = nullptr;
	Physics* m_phys = nullptr;
} s_circle;

static struct
{
	BoxGeometry* m_geo = nullptr;
	Physics* m_phys = nullptr;
} s_board;

struct SimplexResult
{
	Simplex simplex;
	vector3 witnessPoints[2];
	vector3 searchDirection;
	COLLISION_RESULT result = COLLISION_RESULT_NONE;
	SimplexResult(const Simplex& s) : simplex(s) {}
};

static std::vector<SimplexResult> s_simplexs;
static int s_iterCount = 0;
static CollisionParams s_collisionParams;
static int s_simplexIndex = 0;
static bool s_simplexView = true;

static void ResetSimplex()
{
	s_simplexs.clear();
	Simplex simplex;
	vector3 a_local = s_collisionParams.a->Support({1,0,0}, matrix4());
	vector3 b_local = s_collisionParams.b->Support({-1,0,0}, matrix4());
	simplex.verts.resize(1);
	simplex.verts[0].A = s_collisionParams.aTransform * a_local;
	simplex.verts[0].B = s_collisionParams.bTransform * b_local;
	simplex.verts[0].p = simplex.verts[0].A - simplex.verts[0].B;
	SimplexResult start(simplex);
	start.searchDirection = GetSearchDirection(start.simplex);
	s_simplexs.push_back(start);
	s_simplexIndex = 0;
}

static void ProcessInput(float dt)
{
	Input input = Platform_ConsumeInput();
	Platform_BasicCameraInput(input, dt);

	if (input.CheckKey('T'))
	{
		if (s_simplexs[s_simplexIndex].result != COLLISION_RESULT_NO_OVERLAP)
		{
			s_simplexIndex++;
			if (s_simplexIndex == s_simplexs.size())
			{
				SimplexResult next = Simplex(s_simplexs.back().simplex);
				next.result = DetectCollisionStep(s_collisionParams, next.simplex);
				if (next.simplex.size() < 4)
				{
					next.searchDirection = GetSearchDirection(next.simplex);
				}
				
				if (next.result != COLLISION_RESULT_CONTINUE)
				{
					//GetWitnessPoints(next.simplex, next.witnessPoints[0], next.witnessPoints[1]);
				}
				s_simplexs.push_back(next);
			}
		}
	}
	if (input.CheckKey('G'))
	{
		s_simplexIndex = max(0, s_simplexIndex - 1);
	}
	if (input.CheckKey('R'))
	{
		ResetSimplex();
	}

	// move and rotate object
	if (input.CheckKey(ARROW_KEY_UP))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 1.0f, 0.0f));
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_DOWN))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, -1.0f, 0.0f));
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_LEFT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 0.0f, 1.0f));
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (input.CheckKey(ARROW_KEY_RIGHT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 0.0f, -1.0f));
		s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
		ResetSimplex();
	}
	if (input.CheckKey('Z'))
	{
		s_simplexView = !s_simplexView;
		Platform_ResetCamera();
	}
}

static void Update(float dt)
{
	ProcessInput(dt);
}

static void Draw()
{
	glPointSize(4.f);

	const auto& result = s_simplexs[s_simplexIndex];
	if (result.result == COLLISION_RESULT_OVERLAP || result.result == COLLISION_RESULT_NO_OVERLAP)
	{
		const vector4 witnessPointAColor = { 0.0f, 1.0f, 0.0f, 1.0f };
		const vector4 witnessPointBColor = { 1.0f, 0.0f, 0.0f, 1.0f };
		const vector4 witnessLineColor   = { 1.0f, 1.0f, 1.0f, 1.0f };
		glBegin(GL_POINTS);
		glColor4fv((GLfloat*)&witnessPointAColor);
		glVertex3fv((GLfloat*)&result.witnessPoints[0]);
		glColor4fv((GLfloat*)&witnessPointBColor);
		glVertex3fv((GLfloat*)&result.witnessPoints[1]);
		glEnd();

		glBegin(GL_LINES);
		glColor4fv((GLfloat*)&witnessLineColor);
		glVertex3fv((GLfloat*)&result.witnessPoints[0]);
		glVertex3fv((GLfloat*)&result.witnessPoints[1]);
		glEnd();
	}

	if (s_simplexView)
	{
		glDisable(GL_LIGHTING);

		GLfloat modelMatrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glPushMatrix();
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&modelMatrix);

		
		// Coordinate Axis Lines
		glBegin(GL_LINES);

		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(-100.f,0.f,0.f);
		glVertex3f(100.f,0.f,0.f);

		glColor3f(0.f, 1.0, 0.f);
		glVertex3f(0.f, -100.f, 0.f);
		glVertex3f(0.f, 100.f, 0.f);

		glColor3f(0.f, 0.f, 1.0f);
		glVertex3f(0.f, 0.f, -100.f);
		glVertex3f(0.f, 0.f, 100.f);

		glEnd();


		const auto& s = result.simplex;
		const vector4 simplexColor = { 0.5f, 0.5f, 0.5f, 0.2f };
		const vector4 searchDirectionColor = { 0.9f, 0.9f, 0.9f, 0.7f };
		switch (s.size())
		{
			case 1:
			{
				glBegin(GL_POINTS);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s.verts[0].p));
				glEnd();

				vector3 midPoint = (s.verts[0].p);
				vector3 searchNorm = result.searchDirection.normalize();
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
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s.verts[0].p));
				glVertex3fv((GLfloat*)(&s.verts[1].p));

				vector3 midPoint = (s.verts[0].p + s.verts[1].p) / 2.0f;
				vector3 searchNorm = result.searchDirection.normalize();
				vector3 searchDirEnd = midPoint + searchNorm * 5.0f;
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)&searchDirectionColor);
				glVertex3fv((GLfloat*)&midPoint);
				glVertex3fv((GLfloat*)&searchDirEnd);
				glEnd();

				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[0].p);
				glColor3f(0.0f, 1.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[1].p);
				glEnd();
			}
			break;

			case 3:
			{


				glBegin(GL_TRIANGLES);
				glColor4fv((GLfloat*)(&simplexColor));
				glVertex3fv((GLfloat*)(&s.verts[0].p));
				glVertex3fv((GLfloat*)(&s.verts[1].p));
				glVertex3fv((GLfloat*)(&s.verts[2].p));
				glEnd();

				vector3 midPoint = (s.verts[0].p + s.verts[1].p + s.verts[2].p) / 3.0f;
				vector3 searchNorm = result.searchDirection.normalize();
				vector3 searchDirEnd = midPoint + searchNorm * 5.0f;
				glBegin(GL_LINES);
				glColor4fv((GLfloat*)&searchDirectionColor);
				glVertex3fv((GLfloat*)&midPoint);
				glVertex3fv((GLfloat*)&searchDirEnd);
				glEnd();

				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[0].p);
				glColor3f(0.0f, 1.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[1].p);
				glColor3f(0.0f, 0.0f, 1.0f);
				glVertex3fv((GLfloat*)&s.verts[2].p);
				glEnd();
			}
			break;

			case 4:
			{
				//  ABC, ACD, DBA, BDC
				glBegin(GL_TRIANGLES);
				glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
				glVertex3fv((GLfloat*)(&s.verts[0].p));
				glVertex3fv((GLfloat*)(&s.verts[1].p));
				glVertex3fv((GLfloat*)(&s.verts[2].p));
					
				glColor4f(0.4f, 0.4f, 0.4f, 1.0f);
				glVertex3fv((GLfloat*)(&s.verts[0].p));
				glVertex3fv((GLfloat*)(&s.verts[2].p));
				glVertex3fv((GLfloat*)(&s.verts[3].p));
					
				glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
				glVertex3fv((GLfloat*)(&s.verts[3].p));
				glVertex3fv((GLfloat*)(&s.verts[1].p));
				glVertex3fv((GLfloat*)(&s.verts[0].p));
					
				glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
				glVertex3fv((GLfloat*)(&s.verts[1].p));
				glVertex3fv((GLfloat*)(&s.verts[3].p));
				glVertex3fv((GLfloat*)(&s.verts[2].p));
				glEnd();

					
				vector3 origin;
				vector3 ABC_center = (s.verts[0].p + s.verts[1].p + s.verts[2].p) / 3.0f;
				vector3 ACD_center = (s.verts[0].p + s.verts[2].p + s.verts[3].p) / 3.0f;
				vector3 DBA_center = (s.verts[3].p + s.verts[1].p + s.verts[0].p) / 3.0f;
				vector3 BDC_center = (s.verts[1].p + s.verts[2].p + s.verts[3].p) / 3.0f;
				glBegin(GL_LINES);
				glVertex3fv((GLfloat*)&ABC_center);
				glVertex3fv((GLfloat*)&origin);
				glVertex3fv((GLfloat*)&ACD_center);
				glVertex3fv((GLfloat*)&origin);
				glVertex3fv((GLfloat*)&DBA_center);
				glVertex3fv((GLfloat*)&origin);
				glVertex3fv((GLfloat*)&BDC_center);
				glVertex3fv((GLfloat*)&origin);
				glEnd();

				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[0].p);
				glColor3f(0.0f, 1.0f, 0.0f);
				glVertex3fv((GLfloat*)&s.verts[1].p);
				glColor3f(0.0f, 0.0f, 1.0f);
				glVertex3fv((GLfloat*)&s.verts[2].p);
				glColor3f(1.0f, 0.0f, 1.0f);
				glVertex3fv((GLfloat*)&s.verts[3].p);
				glEnd();
			}
			break;
		}

		glPopMatrix();
	}
	else
	{
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
			m = m.t();
			DrawParams p;
			p.drawType = DrawType_Triangles;
			p.color = vector4(0.0f, 0.2f, 0.0f, 0.7f);
			s_circle.m_geo->Draw(m, &p);
		}

		{
			matrix4 m = s_board.m_phys->GetTransform();
			m = m.t();
			DrawParams p;
			p.drawType = DrawType_Triangles;
			p.color = vector4(0.2f, 0.0f, 0.0f, 0.7f);
			s_board.m_geo->Draw(m, &p);
		}
	}
}

static void CreateCircle()
{
	constexpr float CIRCLE_SIZE = 5.0f;

	s_circle.m_geo = new SphereGeometry(CIRCLE_SIZE);

	StaticPhysicsData physData;
	physData.m_gravity = { 0.0f, 0.0f, 0.0f };
	//physData.m_initialPosition = { -1.58030558f, -6.79232216f, -0.412327528f };
	//physData.m_initialRotation = { 0.359199494f, 20.3241673f, -0.861259818f };
	//physData.m_initialPosition = { 0.f, 8.72999573f, 0.f };
	//physData.m_initialRotation = { 0.f, 20.f, 0.f };
	physData.m_initialPosition = { 0.f, 10.f, 0.f };
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
	s_circle.m_phys = new Physics(s_circle.m_geo, physData);
}

static void CreateBoard()
{
	s_board.m_geo = new BoxGeometry(50.f,50.f,2.f);

	constexpr float CIRCLE_SIZE = 5.0f;

	StaticPhysicsData physData;
	physData.m_elasticity = 0.0f;
	physData.m_gravity = { 0.0f,-1.0f,0.0f };
	physData.m_collisionResponseType = COLLISION_RESPONSE_NONE;
	physData.m_initialPosition = { 0.0f, -1.0f, 0.0f };
	physData.m_initialRotation = { 0.0f };

	s_board.m_phys = new Physics(s_board.m_geo, physData);
}

void TestSimplex()
{
	CreateCircle();
	CreateBoard();

	s_collisionParams.a = s_circle.m_geo;
	s_collisionParams.aTransform = s_circle.m_phys->GetTransform();
	s_collisionParams.b = s_board.m_geo;
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