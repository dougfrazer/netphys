#pragma once

// interface to platform specific code
#include "vector.h"
#include <cstdint>

//-------------------------------------------------------------------------------------------------
// platform functions
//-------------------------------------------------------------------------------------------------
typedef void (*platformUpdateFn)(float);
typedef void (*platformDrawFn)(void);

struct PlatformParams
{
    int windowSize[2];
    int windowPos[2];
    platformUpdateFn updateCallback;
    platformDrawFn drawCallback;
};
void Platform_Run(const PlatformParams& fns);


//-------------------------------------------------------------------------------------------------
// input functions
//-------------------------------------------------------------------------------------------------

// These are keys we are manually and explicitly mapping
enum SPECIAL_INPUT_KEY
{
    LEFT_MOUSE,
    RIGHT_MOUSE,
    MIDDLE_MOUSE,

    MOUSE_INPUT_FIRST = LEFT_MOUSE,
    MOUSE_INPUT_LAST = MIDDLE_MOUSE,

    ARROW_KEY_LEFT,
    ARROW_KEY_RIGHT,
    ARROW_KEY_DOWN,
    ARROW_KEY_UP,

    ARROW_INPUT_FIRST = ARROW_KEY_LEFT,
    ARROW_INPUT_LAST = ARROW_KEY_UP,
};

bool Platform_InputIsDown(char key);
bool Platform_InputIsDown(SPECIAL_INPUT_KEY key);
bool Platform_InputChangedDown(char key);
bool Platform_InputChangedDown(SPECIAL_INPUT_KEY key);

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// camera functions
//-------------------------------------------------------------------------------------------------
vector3 Platform_GetCameraPos();
vector3 Platform_GetCameraLookAt();
vector3 Platform_GetCameraOrbit();

void Platform_SetCameraPos(const vector3& pos);
void Platform_SetCameraLookAt(const vector3& lookAt);
void Platform_SetCameraOrbit(const vector3& orbit);

void Platform_ResetCamera();

void Platform_BasicCameraInput(float dt);