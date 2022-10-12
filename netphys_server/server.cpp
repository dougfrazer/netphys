#include <Windows.h>
#include "network_s.h"


#include "../netphys_common/world.h"
#include "../netphys_common/common.h"

#include <chrono>

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
static constexpr DWORD TICK_RATE = 100; // milliseconds
static bool s_running = true;
static std::chrono::steady_clock::time_point s_startTime;
static std::chrono::steady_clock::time_point s_now;

//-------------------------------------------------------------------------------------------------
static void WriteWorldUpdate()
{
    WorldStateUpdatePacket p;
    p.now = (float)((s_now - s_startTime).count() / (1000.f*1000.f));
    for (int i = 0; i < 10; i++)
    {
        const Object& o = World::Get()->GetInteract(i);
        dQuaternion rotQuat;
        if (!o.m_geometry)
        {
            p.interacts[i].valid = false;
            continue;
        }
            
        p.interacts[i].valid = true;

        dGeomGetQuaternion(o.m_geometry, rotQuat);
        p.interacts[i].orientation[0] = (float)rotQuat[0];
        p.interacts[i].orientation[1] = (float)rotQuat[1];
        p.interacts[i].orientation[2] = (float)rotQuat[2];
        p.interacts[i].orientation[3] = (float)rotQuat[3];
        const dReal* pos = dGeomGetPosition(o.m_geometry);
        p.interacts[i].position[0] = (float)pos[0];
        p.interacts[i].position[1] = (float)pos[1];
        p.interacts[i].position[2] = (float)pos[2];
    }
    Net_S_SendToAllClients((char*)&p, sizeof(WorldStateUpdatePacket));
}

//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------
int main(int argc, char* argv)
{
    World::Get()->Init();

    World::Get()->Create();

    Net_S_Init();

    s_startTime = std::chrono::high_resolution_clock::now();
    s_now = s_startTime;
    Sleep(TICK_RATE);
    while (s_running)
    {
        auto start = std::chrono::high_resolution_clock::now();
        float dt = (start - s_now).count() / (1000.f*1000.f); // returns nano seconds
        s_now = start;

        // do updates
        WriteWorldUpdate();
        Net_S_Update();
        World::Get()->Update(dt);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end-start).count() / (1000.f*1000.f);
        DWORD sleepDuration = min(10, TICK_RATE - (DWORD)duration);
        Sleep(sleepDuration);
    }

    Net_S_Deinit();

    World::Get()->Deinit();
}