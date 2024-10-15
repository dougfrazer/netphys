#include "test.h"

#include <chrono>
#include <thread>
#include "platform.h"
#include "physics.h"
#include "lib.h"
#include "simplex.h"
#include "physics_shape.h"

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

static bool s_run = false;
static bool s_tickOnce = false;

static void HandleInput(float dt)
{
    Platform_BasicCameraInput(dt);
	
    if (Platform_InputIsDown('T'))
	{
		s_tickOnce = true;
	}

    if (Platform_InputIsDown('P'))
    {
        s_run = true;
    }
}

static void Update(float dt)
{
    HandleInput(dt);

    if (s_run || s_tickOnce)
    {
        Physics_Update(dt);
    }
    
    s_tickOnce = false;
}

static void Draw()
{
    {
        matrix4 m = s_circle.m_phys->GetTransform();
        m.transpose();
        s_circle.m_shape->Draw(m);
    }
    
    {
        matrix4 m = s_board.m_phys->GetTransform();
        m.transpose();
        DrawParams p;
        p.drawType = DrawType_Triangles;
        p.color = { 1.0f, 0.0f, 0.0f, 0.5f };
        s_board.m_shape->Draw(m, &p);
    }
}

static void CreateCircle()
{
    s_circle.m_shape = new MeshPhysicsShape();
    constexpr float CIRCLE_SIZE = 5.0f;
    s_circle.m_shape->CreateSphere(CIRCLE_SIZE);

    StaticPhysicsData physData;
    physData.m_gravity = { 0.0f,-9.8f, 0.0f };
    physData.m_initialPosition = { 0.0f, 20.0f, 0.0f };
    physData.m_initialRotation = { 0.0f, 20.0f, 0.0f };
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
    s_board.m_shape->CreateBox(50.f, 50.f, 2.f);

    StaticPhysicsData physData;
    physData.m_elasticity = 0.0f;
    physData.m_gravity = { 0.0f,0.0f,0.0f };
    physData.m_collisionResponseType = COLLISION_RESPONSE_NONE;
    physData.m_initialPosition = { 0.0f, -1.0f, 0.0f };
    physData.m_initialRotation = { 0.0f };

    s_board.m_phys = new Physics(s_board.m_shape, physData);
}

void TestPhysics()
{
    CreateCircle();
    CreateBoard();

	PlatformParams p;
	p.windowPos[0] = 100;
	p.windowPos[1] = 100;
	p.windowSize[0] = 640;
	p.windowSize[1] = 640;
	p.updateCallback = Update;
	p.drawCallback = Draw;
    Platform_Run(p);
}