
#include <ode/ode.h>
#include "world.h"
#include "common.h"
#include <vector>

static World s_world;

// world variables
static dWorldID s_worldID;
static dSpaceID s_spaceID;
static dGeomID s_ground;

static dJointGroupID s_contactGroup;
static dThreadingImplementationID s_threading;
static dThreadingThreadPoolID s_threadPool;


// interact box constants
static Object s_interacts[NUM_INTERACTS] = { 0 };

static constexpr float PLAYER_ACCEL = 20.0f; // for now same in all directions



// player constants
static Object s_player = { 0 };
static constexpr dReal PLAYER_SIZE = 2.f;


static constexpr int MAX_CONTACTS = 8;

//-------------------------------------------------------------------------------------------------
World* World::Get()
{
    return &s_world;
}
//-------------------------------------------------------------------------------------------------
void World::Init()
{
    // Create the dynamics world and associated objects
    dInitODE2(0);
    s_worldID = dWorldCreate();
    s_spaceID = dHashSpaceCreate(0);
    s_contactGroup = dJointGroupCreate(0);
    s_threading = dThreadingAllocateMultiThreadedImplementation();
    s_threadPool = dThreadingAllocateThreadPool(8, 0, dAllocateFlagBasicData, NULL);
    dThreadingThreadPoolServeMultiThreadedImplementation(s_threadPool, s_threading);
    dWorldSetStepIslandsProcessingMaxThreadCount(s_worldID, 1);
    dWorldSetStepThreadingImplementation(s_worldID, dThreadingImplementationGetFunctions(s_threading), s_threading);

    // set a bunch of constants/world parameters
    dWorldSetGravity(s_worldID, 0, 0, -GRAVITY);
    dWorldSetCFM(s_worldID, 1e-5); // allow some fudge
    dWorldSetAutoDisableFlag(s_worldID, 1); // auto disable objects at rest
    dWorldSetAutoDisableAverageSamplesCount(s_worldID, 10);
    dWorldSetLinearDamping(s_worldID, 0.00001);
    dWorldSetAngularDamping(s_worldID, 0.005);
    dWorldSetMaxAngularSpeed(s_worldID, 200);
    dWorldSetContactMaxCorrectingVel(s_worldID, 0.1);
    dWorldSetContactSurfaceLayer(s_worldID, 0.001);
}
//-------------------------------------------------------------------------------------------------
void World::Deinit()
{
    dThreadingImplementationShutdownProcessing(s_threading);
    dThreadingFreeThreadPool(s_threadPool);
    dWorldSetStepThreadingImplementation(s_worldID, NULL, NULL);
    dThreadingFreeImplementation(s_threading);

    dJointGroupDestroy(s_contactGroup);
    dSpaceDestroy(s_spaceID);
    dWorldDestroy(s_worldID);
    dCloseODE();
}
//-------------------------------------------------------------------------------------------------
static void NearCallback(void*, dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    // exit without doing anything if the two bodies are connected by a joint
    if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
        return;

    const bool isGround = ((o1 == s_ground) || (o2 == s_ground));

    dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
    if (int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact))) {
        for (int i = 0; i < numc; i++) {
            contact[i].surface.mode = dContactBounce | dContactSoftCFM;
            contact[i].surface.mu = dInfinity;
            contact[i].surface.mu2 = 0;
            if (isGround)
            {
                contact[i].surface.mode = dContactBounce | dContactSoftCFM;
                contact[i].surface.soft_cfm = 0;
                contact[i].surface.bounce = .5;
                contact[i].surface.bounce_vel = .3;
            }
            else if (contact[i].geom.g2 == s_player.m_geomID)
            {
                // This is a convenience function: given a vector, it finds other 2 perpendicular vectors
                dVector3 motiondir1, motiondir2;
                dPlaneSpace(contact[i].geom.normal, motiondir1, motiondir2);
                for (int i = 0; i < 3; ++i)
                    contact[i].fdir1[i] = motiondir1[i];

                dVector3 mov2_vel = { 0.2, 0.1, 0.25 };
                for (int i = 0; i < 3; i++) { mov2_vel[i] *= 100.f; }
                contact[i].surface.motion1 = dCalcVectorDot3(mov2_vel, motiondir1);

                contact[i].surface.mode = dContactBounce | dContactSoftCFM | dContactMotion1;
                contact[i].surface.mu = 50.0; // was: dInfinity
                contact[i].surface.soft_cfm = 0.04;
                contact[i].surface.bounce = .5;
            }
            else
            {
                contact[i].surface.mode = dContactBounce | dContactSoftCFM;
                contact[i].surface.mu = 50.0; // was: dInfinity
                contact[i].surface.soft_cfm = 0.04;
                contact[i].surface.bounce = .5;
            }
            dJointID c = dJointCreateContact(s_worldID, s_contactGroup, contact + i);
            dJointAttach(c, b1, b2);
        }
    }
}
//-------------------------------------------------------------------------------------------------
void World::Update(float dt)
{
    dSpaceCollide(s_spaceID, 0, &NearCallback);
    dWorldQuickStep(s_worldID, dt);
    dJointGroupEmpty(s_contactGroup);
}
//-------------------------------------------------------------------------------------------------
void World::Create()
{
    // just a plane for now
    s_ground = dCreatePlane(s_spaceID, 0, 0, 1, 0);
}
//-------------------------------------------------------------------------------------------------
void World::Reset()
{
    // cleanup anything if we're resetting an active world
    for (int i = 0; i < NUM_ROWS_COLS * NUM_ROWS_COLS; i++)
    {
        if (s_interacts[i].m_bodyID)
        {
            dBodyDestroy(s_interacts[i].m_bodyID);
            dGeomDestroy(s_interacts[i].m_geomID);
        }
        s_interacts[i].m_bodyID = 0;
        s_interacts[i].m_geomID = 0;
    }
    if (s_player.m_bodyID)
    {
        dBodyDestroy(s_player.m_bodyID);
        dGeomDestroy(s_player.m_geomID);
        s_player.m_bodyID = 0;
        s_player.m_geomID = 0;
    }

    // initialize the interactibles
    for (int i = 0; i < NUM_ROWS_COLS; i++)
    {
        for (int j = 0; j < NUM_ROWS_COLS; j++)
        {
            Object& obj = s_interacts[i * NUM_ROWS_COLS + j];
            obj.m_bodyID = dBodyCreate(s_worldID);
            dBodySetPosition(obj.m_bodyID, i - NUM_ROWS_COLS / 2, j - NUM_ROWS_COLS / 2, BOX_SIZE);
            dMatrix3 R;
            dRSetIdentity(R);

            dMass mass;
            dMassSetBox(&mass, DENSITY, BOX_SIZE, BOX_SIZE, BOX_SIZE);
            obj.m_geomID = dCreateBox(s_spaceID, BOX_SIZE, BOX_SIZE, BOX_SIZE);

            dGeomSetBody(obj.m_geomID, obj.m_bodyID);
            dBodySetMass(obj.m_bodyID, &mass);
        }
    }
}
//-------------------------------------------------------------------------------------------------
void World::Start()
{
    dAllocateODEDataForThread(dAllocateMaskAll);

    Reset();
}
//-------------------------------------------------------------------------------------------------
void World::CreatePlayer()
{
    s_player.m_bodyID = dBodyCreate(s_worldID);
    dBodySetAutoDisableFlag(s_player.m_bodyID, 0);
    dBodySetPosition(s_player.m_bodyID, 0, 0, 5.f);
    dMatrix3 R;
    dRSetIdentity(R);
    s_player.m_geomID = dCreateSphere(s_spaceID, PLAYER_SIZE + .2f);  // make the physics sphere a little bigger
    dGeomSetBody(s_player.m_geomID, s_player.m_bodyID);

    dMass mass;
    dMassSetSphere(&mass, DENSITY, PLAYER_SIZE);
    dBodySetMass(s_player.m_bodyID, &mass);
}
//-------------------------------------------------------------------------------------------------
const Object& World::GetPlayer()
{
    return s_player;
}
//-------------------------------------------------------------------------------------------------
const Object& World::GetInteract(int index)
{
    return s_interacts[index];
}
//-------------------------------------------------------------------------------------------------
void World::HandleInputs(int inputMask)
{
    bool wantsJump = false;

    if (World::Get()->GetPlayer().m_bodyID)
    {
        float a[3] = { 0.0f };
        if (inputMask & INPUT_MOVE_FORWARD)
        {
            a[0] += PLAYER_ACCEL;
        }
        if (inputMask & INPUT_MOVE_BACKWARD)
        {
            a[0] -= PLAYER_ACCEL;
        }
        if (inputMask & INPUT_MOVE_LEFT)
        {
            a[1] += PLAYER_ACCEL;
        }
        if (inputMask & INPUT_MOVE_RIGHT)
        {
            a[1] -= PLAYER_ACCEL;
        }
        if (inputMask & INPUT_SPACE)
        {
            a[2] += PLAYER_ACCEL;
        }
        dMass m;
        dBodyGetMass(World::Get()->GetPlayer().m_bodyID, &m);
        dReal f[3] = { a[0] * m.mass, a[1] * m.mass, a[2] * m.mass };
        dBodyAddForce(World::Get()->GetPlayer().m_bodyID, f[0], f[1], f[2]);
    }
    else
    {
        if (inputMask & INPUT_SPACE)
        {
            World::Get()->CreatePlayer();
        }
    }
}