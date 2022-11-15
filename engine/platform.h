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
enum MOUSE_INPUT
{
    LEFT_BUTTON   = (1 << 0),
    MIDDLE_BUTTON = (1 << 1),
    RIGHT_BUTTON  = (1 << 2),

    MOUSE_MOVE    = (1 << 3),

    NUM_MOUSE_INPUT = 4
};
static_assert(NUM_MOUSE_INPUT < 8, "need additional bits for mouse input");

static constexpr int START_CHAR = '0';
static constexpr int END_CHAR = 'Z';

enum KEY_INPUT
{
    ARROW_KEY_LEFT = 1,
    ARROW_KEY_RIGHT,
    ARROW_KEY_DOWN,
    ARROW_KEY_UP,

    NUM_KEY_INPUT
};
static_assert(NUM_KEY_INPUT + (END_CHAR - START_CHAR) < 64, "Not enough bits to represent key input");

struct Input
{
    char mouseInput;
    uint64_t keyInput;
    int x;
    int y;

    bool CheckKey(char key) const { 
        if (key >= START_CHAR && key <= END_CHAR)
        {
            int index = key - START_CHAR;
            return (keyInput & (1ull << index)) > 0;
        }
            
        return false;
    }
    bool CheckKey(KEY_INPUT key) const {
        int index = key + (END_CHAR - START_CHAR);
        return (keyInput & (1ull << index)) > 0;
    }
};

Input Platform_ConsumeInput();
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

void Platform_BasicCameraInput(const Input& input, float dt);