#pragma once

#ifndef dDOUBLE
#define dDOUBLE
#endif

#define arrsize(x) sizeof(x) / sizeof(*x)

enum PLAYER_INPUT
{
    INPUT_SPACE            = (1 << 0),
    INPUT_RESET_WORLD      = (1 << 1),

    INPUT_MOVE_FORWARD     = (1 << 2),
    INPUT_MOVE_BACKWARD    = (1 << 3),
    INPUT_MOVE_LEFT        = (1 << 4),
    INPUT_MOVE_RIGHT       = (1 << 5),
};

static constexpr int NUM_ROWS_COLS = 20;
static constexpr int NUM_INTERACTS = NUM_ROWS_COLS * NUM_ROWS_COLS;


//
// Packets
//
static constexpr int INPUT_PACKET_ID = 123456;
static constexpr int WORLD_STATE_UPDATE_ID = 2342144;
static constexpr int HANDLE_WORLD_RESET_ID = 2389;

struct Packet
{
    Packet(int _id) : id(_id) {}
    int GetType() const { return id; }
private:
    int id;
};

struct InputPacket : public Packet
{
    InputPacket() : Packet(INPUT_PACKET_ID) {}
    int value;
};

struct WorldStateUpdatePacket : public Packet
{
    WorldStateUpdatePacket() : Packet(WORLD_STATE_UPDATE_ID) {}
    struct ObjectUpdate
    {
        float position[3];
        float orientation[4];
        bool valid;
    };
    ObjectUpdate interacts[10];
    float now = 0.0f;
};

struct HandleWorldStateResetPacket : public Packet
{
    HandleWorldStateResetPacket() : Packet(HANDLE_WORLD_RESET_ID) {}
};