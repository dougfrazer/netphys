#include "world_s.h"

#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/lib.h"
#include "../netphys_common/log.h"

#include "network_s.h"
#include "objectmanager_s.h"
#include <vector>

struct CommandFrame
{
    FrameNum id;
    double timeMs;
    std::vector<CommandFrameObject> objects;
};

std::vector<CommandFrame> s_commandFrames;
static int s_frameCounter = 1;
static constexpr float MAX_COMMAND_FRAME_TIME = 5000.f; // only keep 5 seconds of command frames
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
    for (Object* obj = ObjectManager_S_GetFirst(); obj != nullptr; obj = ObjectManager_S_GetNext(obj))
    {
        dBodyID bodyID = obj->GetBodyID();
        if (bodyID)
        {
            const dReal* pos = dBodyGetPosition(bodyID);
            const dReal* rot = dBodyGetQuaternion(bodyID);

            CommandFrameObject frameObj(obj->GetGUID());
            frameObj.pos[0] = (float)pos[0];
            frameObj.pos[1] = (float)pos[1];
            frameObj.pos[2] = (float)pos[2];
            frameObj.rot[0] = (float)rot[0];
            frameObj.rot[1] = (float)rot[1];
            frameObj.rot[2] = (float)rot[2];
            frameObj.rot[3] = (float)rot[3];
            frameObj.isValid = true;
            frameObj.isEnabled = dBodyIsEnabled(bodyID);
            newFrame.objects.push_back(frameObj);
        }
    }
    s_commandFrames.push_back(newFrame);

    //
    // erase old command frames
    //
    auto it = s_commandFrames.begin();
    for (; it != s_commandFrames.end(); ++it)
    {
        if (now - it->timeMs < MAX_COMMAND_FRAME_TIME)
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
void World_S_FillNewConnectionMessage(ClientNewConnection* msg)
{
    if (!s_commandFrames.size())
    {
        LOG_ERROR("Tried to send world state before any command frames were generated");
        return;
    }
    const CommandFrame& frame = s_commandFrames.back();
    msg->PutID(frame.id);
    msg->PutTimeMs(frame.timeMs);
    msg->PutNumObjects(frame.objects.size());
    for (const auto& obj : frame.objects)
    {
        msg->PutFrameObject(obj);
    }
    msg->Finalize();
}
//-------------------------------------------------------------------------------------------------
void World_S_FillWorldUpdateMessage(ClientWorldStateUpdatePacket* msg, unsigned int lastAckedFrame)
{
    if (!s_commandFrames.size())
    {
        LOG_ERROR("Tried to send world state before any command frames were generated");
        return;
    }

    const CommandFrame& frame = s_commandFrames.back();
    msg->PutID(frame.id);
    msg->PutTimeMs(frame.timeMs);
    msg->PutNumObjects(frame.objects.size());
    for (const auto& obj : frame.objects)
    {
        msg->PutFrameObject(obj);
    }
    msg->Finalize();
}