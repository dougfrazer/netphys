#include "world_s.h"

#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/lib.h"
#include "../netphys_common/log.h"

#include "network_s.h"
#include <vector>

std::vector<CommandFrame> s_commandFrames;
static int s_frameCounter = 1;
static constexpr float MAX_COMMAND_FRAME_SIZE = 5000.f; // only keep 5 seconds of command frames
//-------------------------------------------------------------------------------------------------
void World_S_Update(double now)
{
    // TODO: need better/real data structures eventually
    //
    // store off the state of the world
    //
    CommandFrame newFrame;
    newFrame.id = s_frameCounter++; // hope this never overflows... at 10ms frames it'll take >400 days don't expect servers to be up that long
    newFrame.timeMs = now;
    for (int i = 0; i < NUM_INTERACTS; i++)
    {
        const Object& obj = World::Get()->GetInteract(i);
        const dReal* pos = dGeomGetPosition(obj.m_geomID);
        dQuaternion rot;
        dGeomGetQuaternion(obj.m_geomID, rot);
        newFrame.objects[i].pos[0] = (float)pos[0];
        newFrame.objects[i].pos[1] = (float)pos[1];
        newFrame.objects[i].pos[2] = (float)pos[2];
        newFrame.objects[i].rot[0] = (float)rot[0];
        newFrame.objects[i].rot[1] = (float)rot[1];
        newFrame.objects[i].rot[2] = (float)rot[2];
        newFrame.objects[i].rot[3] = (float)rot[3];
    }
    s_commandFrames.push_back(newFrame);

    //
    // erase old command frames
    //
    auto it = s_commandFrames.begin();
    for (; it != s_commandFrames.end(); ++it)
    {
        if (now - it->timeMs < MAX_COMMAND_FRAME_SIZE)
        {
            break;
        }
    }
    if (it != s_commandFrames.begin())
    {
        s_commandFrames.erase(s_commandFrames.begin(), it);
    }
}

//-------------------------------------------------------------------------------------------------
// Initial full state of the world packet
void World_S_FillWorldState(struct ClientNewConnection* msg)
{
    if (!s_commandFrames.size())
    {
        //WORLD_ERROR("Tried to send world state before any command frames were generated");
        return;
    }

    msg->frame = s_commandFrames.back();
}
//-------------------------------------------------------------------------------------------------
// incremental updates from last acked state
void World_S_FillWorldStateUpdate(ClientWorldStateUpdatePacket* msg, FrameNum lastAckedFrame)
{
    // TODO: use lastAckedTime to only send the changes since then
    if (!s_commandFrames.size())
    {
        //WORLD_ERROR("Tried to send world state before any command frames were generated");
        return;
    }
    
    // TEMP DEBUG CODE
    //double lastAckedFrameTime = 0.0f;
    //for (const auto& frame : s_commandFrames)
    //{
    //    if (frame.id == lastAckedFrame)
    //    {
    //        lastAckedFrameTime = frame.timeMs;
    //        break;
    //    }
    //}
    //LOG("would have to compute delta between %d@%.2f and %d@%.2f (%d frames, %.2f ms)", lastAckedFrame, lastAckedFrameTime, s_commandFrames.back().id, s_commandFrames.back().timeMs, s_commandFrames.back().id- lastAckedFrame, s_commandFrames.back().timeMs - lastAckedFrameTime);
    // END TEMP DEBUG CODE
    msg->frame = s_commandFrames.back();
}
//-------------------------------------------------------------------------------------------------


void World_S_HandleInputs(int inputMask)
{
    if (inputMask & INPUT_RESET_WORLD)
    {
        // broadcast a reset world to everyone?
    }
    World::Get()->HandleInputs(inputMask);
}