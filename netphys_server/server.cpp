#include <Windows.h>
#include "network_s.h"


#include "../netphys_common/world.h"
#include "../netphys_common/common.h"
#include "../netphys_common/log.h"

#include "world_s.h"

#include <chrono>

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
static constexpr DWORD TICK_RATE = 100; // milliseconds
static bool s_running = true;
static std::chrono::steady_clock::time_point s_startTime;
static std::chrono::steady_clock::time_point s_now;



//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------
int main(int argc, char* argv)
{
    Log_Init();
    World::Get()->Init();

    World::Get()->Create();

    Net_S_Init();

    World::Get()->Start();

    s_startTime = std::chrono::high_resolution_clock::now();
    s_now = s_startTime;
    Sleep(TICK_RATE);
    while (s_running)
    {
        auto start = std::chrono::high_resolution_clock::now();
        float dt = (start - s_now).count() / (1000.l * 1000.l * 1000.l);// nanoseconds to seconds
        s_now = start;

        const double now = (s_now - s_startTime).count() / (1000.f * 1000.f);
        
        // do updates
        Net_S_Update();
        World_S_Update(now);
        World::Get()->Update(dt); 
        

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end-start).count() / (1000.f*1000.f);
        DWORD sleepDuration = min(10, TICK_RATE - (DWORD)duration);
        Sleep(sleepDuration);
    }

    Net_S_Deinit();

    World::Get()->Deinit();
    Log_Deinit();
}