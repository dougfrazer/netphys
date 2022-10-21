#include "player.h"

#include "../netphys_common/common.h"

#include <ode/ode.h>


void Player::HandleInputsInternal(dBodyID bodyID, int inputMask)
{
    if(!bodyID)
        return;

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
    dBodyGetMass(bodyID, &m);
    dReal f[3] = { a[0] * m.mass, a[1] * m.mass, a[2] * m.mass };
    dBodyAddForce(bodyID, f[0], f[1], f[2]);
}