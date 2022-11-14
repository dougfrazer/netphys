#include "test.h"

#include <chrono>
#include <thread>
#include "platform.h"
#include "physics.h"
#include "lib.h"
#include "simplex.h"
#include <vector>

#include "windows.h"
#include <gl/GL.h>
#include <gl/GLU.h>

struct TestObj
{
	Geometry* m_geo = nullptr;
	Physics* m_phys = nullptr;
};

static TestObj s_circle;
static TestObj s_board;


static float s_fixedTimestep = 1.f / 30.f;
static std::vector<Simplex> s_simplexArray;
static int s_iterCount = 0;
static CollisionParams s_collisionParams;
static int s_simplexIndex = 0;
static bool s_simplexView = true;
static std::vector<vector3> s_witnessPoints;

static void ResetSimplex()
{
	vector3 a_local = vector3(s_circle.m_geo->m_vertexList[0]);
	vector3 b_local = vector3(s_board.m_geo->m_vertexList[0]);
	Simplex simplex;
	simplex.verts[0].A = a_local + s_circle.m_phys->GetPosition();
	simplex.verts[0].B = b_local + s_board.m_phys->GetPosition();
	simplex.verts[0].indexA = 0;
	simplex.verts[0].indexB = 0;
	simplex.verts[0].p = simplex.verts[0].A - simplex.verts[0].B;
	simplex.count = 1;
	s_simplexArray.push_back(simplex);
	s_simplexIndex = 0;
	s_witnessPoints.clear();
}

static void ProcessInput(float dt)
{
	Input input = Platform_ConsumeInput();
	Platform_BasicCameraInput(input, dt);

	if (input.CheckKey('T'))
	{
		s_simplexIndex++;
		if (s_simplexIndex == s_simplexArray.size())
		{
			Simplex next = s_simplexArray.back();
			COLLISION_STEP s = DetectCollisionStep(s_collisionParams, next, vector3());
			if (s == COLLISION_STEP_SUCCESS)
			{
				vector3 a,b;
				GetWitnessPoints(next, a, b);
				s_witnessPoints.push_back(a);
				s_witnessPoints.push_back(b);
			}
			else
			{
				s_witnessPoints.clear();
			}
			s_simplexArray.push_back(next);
		}	
	}
	if (input.CheckKey('G'))
	{
		s_simplexIndex = max(0, s_simplexIndex - 1);
		s_witnessPoints.clear();
	}
	if (input.CheckKey('R'))
	{
		ResetSimplex();
	}

	// move and rotate object
	if (input.CheckKey(ARROW_KEY_UP))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 1.0f, 0.0f));
		s_witnessPoints.clear();
		s_simplexArray.clear();
		s_simplexIndex = 0;
	}
	if (input.CheckKey(ARROW_KEY_DOWN))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, -1.0f, 0.0f));
		s_witnessPoints.clear();
		s_simplexArray.clear();
		s_simplexIndex = 0;
	}
	if (input.CheckKey(ARROW_KEY_LEFT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 0.0f, 1.0f));
		s_witnessPoints.clear();
		s_simplexArray.clear();
		s_simplexIndex = 0;
	}
	if (input.CheckKey(ARROW_KEY_RIGHT))
	{
		s_circle.m_phys->SetPosition(s_circle.m_phys->GetPosition() + vector3(0.0f, 0.0f, -1.0f));
		s_witnessPoints.clear();
		s_simplexArray.clear();
		s_simplexIndex = 0;
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPointSize(4.f);

	if (s_witnessPoints.size())
	{
		const vector4 witnessPointColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		glBegin(GL_POINTS);
		glColor4fv((GLfloat*)&witnessPointColor);
		glVertex3fv((GLfloat*)&s_witnessPoints[0]);
		glVertex3fv((GLfloat*)&s_witnessPoints[1]);
		glEnd();

		glBegin(GL_LINES);
		glColor4fv((GLfloat*)&witnessPointColor);
		vector3 ab = s_witnessPoints[1] - s_witnessPoints[0];
		glVertex3fv((GLfloat*)&ab);
		glEnd();
	}

	if (s_simplexView)
	{
		GLfloat modelMatrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glPushMatrix();
		glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&modelMatrix);

		
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

		if (s_simplexArray.size())
		{
			const auto& s = s_simplexArray[s_simplexIndex];
			const vector4 simplexColor = { 0.5f, 0.5f, 0.5f, 0.2f };
			switch (s.count)
			{
				case 1:
					glBegin(GL_POINTS);
					glColor4fv((GLfloat*)(&simplexColor));
					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glEnd();
					break;
				case 2:
					glBegin(GL_LINES);
					glColor4fv((GLfloat*)(&simplexColor));
					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glVertex3fv((GLfloat*)(&s.verts[1].p));
					glEnd();
					break;
				case 3:
					glBegin(GL_TRIANGLES);
					glColor4fv((GLfloat*)(&simplexColor));
					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glVertex3fv((GLfloat*)(&s.verts[1].p));
					glVertex3fv((GLfloat*)(&s.verts[2].p));
					glEnd();
					break;
				case 4:
					glBegin(GL_TRIANGLES);
					//ADB, ACD, CDB, ABC
					glColor4fv((GLfloat*)(&simplexColor));
					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glVertex3fv((GLfloat*)(&s.verts[3].p));
					glVertex3fv((GLfloat*)(&s.verts[1].p));

					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glVertex3fv((GLfloat*)(&s.verts[2].p));
					glVertex3fv((GLfloat*)(&s.verts[3].p));

					glVertex3fv((GLfloat*)(&s.verts[2].p));
					glVertex3fv((GLfloat*)(&s.verts[3].p));
					glVertex3fv((GLfloat*)(&s.verts[1].p));

					glVertex3fv((GLfloat*)(&s.verts[0].p));
					glVertex3fv((GLfloat*)(&s.verts[1].p));
					glVertex3fv((GLfloat*)(&s.verts[2].p));
					glEnd();
					break;
			}
		}

		glPopMatrix();
	}
	else
	{

		{
			matrix4 m = s_circle.m_phys->GetTransform();
			m = m.t();
			DrawParams p;
			p.drawType = GL_TRIANGLES;
			p.color = vector4(0.0f, 0.2f, 0.0f, 0.7f);
			Platform_DrawGeometry(s_circle.m_geo, m, &p);
		}

		{
			matrix4 m = s_board.m_phys->GetTransform();
			m = m.t();
			DrawParams p;
			p.drawType = GL_TRIANGLES;
			p.color = vector4(0.2f, 0.0f, 0.0f, 0.7f);
			Platform_DrawGeometry(s_board.m_geo, m, &p);
		}
	}
}

static void CreateCircle()
{
	s_circle.m_geo = new Geometry();
	GeoHelpers::CreateIcosahadron(5, s_circle.m_geo);

	constexpr float CIRCLE_SIZE = 5.0f;

	StaticPhysicsData physData;
	physData.m_gravity = { 0.0f, 0.0f, 0.0f };
	physData.m_initialPosition = { 0.0f, 4.41799498f, 0.0f };
	physData.m_initialRotation = { 0.0f, 0.0f, 0.0f };
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
	s_board.m_geo = new Geometry();
	GeoHelpers::CreateCube(50, 2, s_board.m_geo);

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

	ResetSimplex();

	const vector3& destination = vector3();
	s_iterCount = 0;
	s_collisionParams.a = s_circle.m_geo;
	s_collisionParams.aPos = s_circle.m_phys->GetPosition();
	s_collisionParams.b = s_board.m_geo;
	s_collisionParams.bPos = s_board.m_phys->GetPosition();

	PlatformParams p;
	p.windowPos[0] = 100;
	p.windowPos[1] = 100;
	p.windowSize[0] = 640;
	p.windowSize[1] = 640;
	p.updateCallback = Update;
	p.drawCallback = Draw;
	Platform_Run(p);
}