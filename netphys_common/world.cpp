
#include <ode/ode.h>
#include "world.h"
#include "common.h"
#include <vector>

#ifdef _NPCLIENT
#include "../netphys_client/objectmanager_c.h"
#else
#include "../netphys_server/objectmanager_s.h"
#endif

static World s_world;

// world variables
static dWorldID s_worldID;
static dSpaceID s_spaceID;
static dGeomID s_ground;

static dJointGroupID s_contactGroup;
static dThreadingImplementationID s_threading;
static dThreadingThreadPoolID s_threadPool;


// interact box constants
static std::vector<WorldObject*> s_objects;

// world constants
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
            else
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
    for (WorldObject* wo : s_objects)
    {
        dBodyDestroy(wo->m_bodyID);
        dGeomDestroy(wo->m_geomID);
    #ifdef _NPCLIENT
        ObjectManager_C_FreeWorldObject(wo);
    #else
        ObjectManager_S_FreeWorldObject(wo);
    #endif
    }
    s_objects.clear();

    // initialize the interactibles
    for (int i = 0; i < NUM_ROWS_COLS; i++)
    {
        for (int j = 0; j < NUM_ROWS_COLS; j++)
        {
            // super crappy :-/ whatever, figure it out later
            // is it assumed if we're in here as a client we're in standalone??
        #ifdef _NPCLIENT
            WorldObject* wo = ObjectManager_C_CreateWorldObject(GetNewGUID(ObjectType_WorldObject));
        #else
            WorldObject* wo = ObjectManager_S_CreateWorldObject();
        #endif
            wo->Init(float(i - NUM_ROWS_COLS / 2), float(j - NUM_ROWS_COLS / 2), BOX_SIZE);
            s_objects.push_back(wo);
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
dBodyID World::CreateBody()
{
    return dBodyCreate(s_worldID);
}
//-------------------------------------------------------------------------------------------------
dGeomID World::CreateSphere(float radius)
{
    return dCreateSphere(s_spaceID, radius);
}
//-------------------------------------------------------------------------------------------------
dGeomID World::CreateBox(float width, float height, float depth)
{
    return dCreateBox(s_spaceID, width, height, depth);
}
//-------------------------------------------------------------------------------------------------
const std::vector<WorldObject*>& World::GetWorldObjects() const
{
    return s_objects;
}
