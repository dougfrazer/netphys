// A typical simulation will proceed like this:
// 
// 1. Create a dynamics world.
// 2. Create bodies in the dynamics world.
// 3. Set the state(position etc) of all bodies.
// 4. Create joints in the dynamics world.
// 5. Attach the joints to the bodies.
// 6. Set the parameters of all joints.
// 7. Create a collision world and collision geometry objects, as necessary.
// 8. Create a joint group to hold the contact joints.
// 9. Loop:
//   Apply forces to the bodies as necessary.
//   Adjust the joint parameters as necessary.
//   Call collision detection.
//   Create a contact joint for every collision point, and put it in the contact joint group.
//   Take a simulation step.
//   Remove all joints in the contact joint group.
// 10. Destroy the dynamics and collision worlds.

#include <ode/ode.h>
#include <drawstuff/drawstuff.h>

#include "../netphys_common/world.h"
#include "../netphys_common/common.h"
#include "../netphys_common/log.h"
#include <windows.h>
#include <string>
#include <chrono>

#include "network_c.h"
#include "client.h"
#include "world_c.h"
#include "player_c.h"
#include "objectmanager_c.h"

static const float INERT_COLOR[3] = { (float)((0xFF) / 255), (float)((0x8F) / 255), (float)((0x8A) / 255) };
static const float ACTIVE_COLOR[3] = { (float)((0x57) / 255), (float)((0xFF) / 255), (float)((0x70) / 255) };
static const float PLAYER_COLOR[3] = { (float)((0xAB) / 255), (float)((0x43) / 255), (float)((0x0A) / 255) };

unsigned int s_inputMask = 0;
static bool s_standaloneMode = false;

//-------------------------------------------------------------------------------------------------
// Called before the sim loop starts
//-------------------------------------------------------------------------------------------------
void ResetCamera()
{
    static float xyz[3] = { -20.f, 0.f, 8.f };
    static float hpr[3] = { 0.f, -30.f, 0.0000f };
    dsSetViewpoint(xyz, hpr);
}
//-------------------------------------------------------------------------------------------------
void Start()
{
    ResetCamera();
}


//-------------------------------------------------------------------------------------------------
// Render our objects
//-------------------------------------------------------------------------------------------------
void Draw()
{
    dsSetTexture(DS_NONE);

    for (Object* obj = ObjectManager_C_GetFirst(); obj != nullptr; obj = ObjectManager_C_GetNext(obj))
    {
        dBodyID b = obj->GetBodyID();
        if(!b)
            continue;
        dGeomID g = dBodyGetFirstGeom(b);
        if(!g)
            continue;
        // todo: this is crappy... don't we have data on the bodies that got created what their shapes are?
        if (obj->GetGUID().GetType() == ObjectType_WorldObject)
        {
            dVector3 sides;
            const dReal* pos = dGeomGetPosition(g);
            const dReal* R = dGeomGetRotation(g);
            dGeomBoxGetLengths(g, sides);
            
            if (dBodyIsEnabled(b))
            {
                dsSetColor(ACTIVE_COLOR[0], ACTIVE_COLOR[1], ACTIVE_COLOR[2]);
            }
            else
            {
                dsSetColor(INERT_COLOR[0], INERT_COLOR[1], INERT_COLOR[2]);
            }
            dsDrawBoxD(pos, R, sides);
        }
        else if (obj->GetGUID().GetType() == ObjectType_Player)
        {
            const dReal* pos = dGeomGetPosition(g);
            const dReal* R = dGeomGetRotation(g);
            dsSetColor(PLAYER_COLOR[0], PLAYER_COLOR[1], PLAYER_COLOR[2]);
            dsDrawSphereD(pos, R, (float)dGeomSphereGetRadius(g));
        }
    }
}
//-------------------------------------------------------------------------------------------------
// Called before every frame
//-------------------------------------------------------------------------------------------------
static std::chrono::steady_clock::time_point s_now;
void Update(int pause)
{
    auto start = std::chrono::high_resolution_clock::now();
    float dt = (start - s_now).count() / (1000.f * 1000.f); // nano-seconds to milli-seconds
    s_now = start;

    if (s_standaloneMode)
    {
        constexpr float MAX_TICK = 1.0f / 30.0f; // max tick 30fps
        if (dt > MAX_TICK)
        {
            dt = MAX_TICK;
        }

        if (!pause)
        {
            ActivePlayer_C::HandleInputs(s_inputMask);
            s_inputMask = 0;
            World::Get()->Update(dt);
        }
    }
    else
    {
        if (s_inputMask)
        {
            ServerInputPacket inputPacket;
            inputPacket.PutMask(s_inputMask);
            inputPacket.Finalize();
            Net_C_Send(&inputPacket);
            s_inputMask = 0;
        }

        Net_C_Update();
        World_C_Update(dt);
    }


    Draw();
}


//-------------------------------------------------------------------------------------------------
// Called if a command key is pressed
//-------------------------------------------------------------------------------------------------
void HandleInput(int input)
{
    switch (input)
    {
    // cache these off so we can consider the sum of all inputs being pressed, plus inputs from last frame
    case '1':           s_inputMask |= INPUT_RESET_WORLD; break;
    case ' ':           s_inputMask |= INPUT_SPACE; break;
    case 'w': case 'W': s_inputMask |= INPUT_MOVE_FORWARD; break;
    case 'a': case 'A': s_inputMask |= INPUT_MOVE_LEFT; break;
    case 's': case 'S': s_inputMask |= INPUT_MOVE_BACKWARD; break;
    case 'd': case 'D': s_inputMask |= INPUT_MOVE_RIGHT; break;

    default: break;
    }
}

//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    Log_Init("client");
    World::Get()->Init();
    

    if (s_standaloneMode)
    {
        World::Get()->Create();
        World::Get()->Start();
    }
    else
    {
        Net_C_Init();
    }
    
    World_C_Init();

    char exeFileName[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, exeFileName, MAX_PATH);
    std::string f(exeFileName);
    std::string exePath = f.substr(0, f.find_last_of("\\/"));
    std::string texturePath = exePath + "/../ode/drawstuff/textures";

    // run simulation
    dsFunctions fn;
    fn.version = DS_VERSION;
    fn.start = &Start;
    fn.step = &Update;
    fn.command = &HandleInput;
    fn.stop = 0;
    fn.path_to_textures = texturePath.c_str();
    dsSimulationLoop(argc, argv, 640, 480, &fn);

    if (!s_standaloneMode)
    {
        Net_C_Deinit();
    }
    World_C_Deinit();
    World::Get()->Deinit();
    Log_Deinit();
}