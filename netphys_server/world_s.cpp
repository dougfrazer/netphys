#include "world_s.h"

#include "../netphys_common/common.h"
#include "../netphys_common/world.h"

#include "network_s.h"

static constexpr float PLAYER_SPEED = 20.0f; // for now same in all directions

void World_S_HandleInput(int inputMask)
{
    bool wantsJump = false;
    if(inputMask & INPUT_RESET_WORLD)
    {
        World::Get()->Reset();
        HandleWorldStateResetPacket p;
        Net_S_SendToAllClients((char*)&p, sizeof(HandleWorldStateResetPacket));
    }

    if (World::Get()->GetPlayer().m_bodyID)
    {
        float v[3] = { 0.0f };
        if (inputMask & INPUT_MOVE_FORWARD)
        {
            v[0] += PLAYER_SPEED;
        }
        if (inputMask & INPUT_MOVE_BACKWARD)
        {
            v[0] -= PLAYER_SPEED;
        }
        if (inputMask & INPUT_MOVE_LEFT)
        {
            v[1] += PLAYER_SPEED;
        }
        if (inputMask & INPUT_MOVE_RIGHT)
        {
            v[1] -= PLAYER_SPEED;
        }
        if (inputMask & INPUT_SPACE)
        {
            v[2] += PLAYER_SPEED;
        }
        dMass m;
        dBodyGetMass(World::Get()->GetPlayer().m_bodyID, &m);
        // f = m*v
        dBodyAddForce(World::Get()->GetPlayer().m_bodyID, m.mass * v[0], m.mass * v[1], m.mass * v[2]);
    }
    else
    {
        if (inputMask & INPUT_SPACE)
        {
            World::Get()->CreatePlayer();
        }
    }
}

