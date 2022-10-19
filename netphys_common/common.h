#pragma once

#ifndef dDOUBLE
#define dDOUBLE
#endif

#define arrsize(x) sizeof(x) / sizeof(*x)
#define lerp(x, x0, x1, y0, y1)  ((fabsf(x0-x1)<0.000001) ? x0 : y0 + ((float)(y1 - y0) * ((x - x0) / (float)(x1 - x0))))

enum PLAYER_INPUT
{
    INPUT_SPACE            = (1 << 0),
    INPUT_RESET_WORLD      = (1 << 1),

    INPUT_MOVE_FORWARD     = (1 << 2),
    INPUT_MOVE_BACKWARD    = (1 << 3),
    INPUT_MOVE_LEFT        = (1 << 4),
    INPUT_MOVE_RIGHT       = (1 << 5),
};

static constexpr int NUM_ROWS_COLS = 3;
static constexpr int NUM_INTERACTS = NUM_ROWS_COLS * NUM_ROWS_COLS;

// world constants
static constexpr float GRAVITY = 9.8f; // m/s^2
static constexpr float BOX_SIZE = 0.5f;
static constexpr float DENSITY = 5.0f;
static constexpr float TOTAL_MASS = DENSITY * BOX_SIZE * BOX_SIZE * BOX_SIZE;

//
// Packets
//
// this is crappy and doesn't consider endianness or anything
// its just an initial implementation to get stood up quickly

struct Packet
{
    Packet(int _id) : id(_id) {}
    int GetType() const { return id; }
private:
    int id;
};

//
// to client
//
static constexpr int CLIENT_WORLD_STATE_UPDATE_ID = 2342144;
static constexpr int CLIENT_HANDLE_WORLD_RESET_ID = 2389;
static constexpr int CLIENT_NEW_CONNECTION_ID = 2342341;

struct CommandFrameObject
{
    float pos[3];
    float rot[4];
};
typedef unsigned int FrameNum;
struct CommandFrame
{
    FrameNum id;
    double timeMs;
    CommandFrameObject objects[NUM_INTERACTS];
};

struct ClientNewConnection : public Packet
{
    ClientNewConnection() : Packet(CLIENT_NEW_CONNECTION_ID) {}

    CommandFrame frame;
};


struct ClientWorldStateUpdatePacket : public Packet
{
    ClientWorldStateUpdatePacket() : Packet(CLIENT_WORLD_STATE_UPDATE_ID) {}

    CommandFrame frame;
};

struct ClientHandleWorldStateResetPacket : public Packet
{
    ClientHandleWorldStateResetPacket() : Packet(CLIENT_HANDLE_WORLD_RESET_ID) {}
};


//
// to server
//
static constexpr int SERVER_NEW_CONNECTION_ID = 349898;
static constexpr int SERVER_NEW_CONNECTION_ACK_ID = 438798;
static constexpr int SERVER_WORLD_UPDATE_ACK_ID = 3478922;
static constexpr int SERVER_INPUT_PACKET_ID = 123456;

struct ServerNewConnection : public Packet
{
    ServerNewConnection() : Packet(SERVER_NEW_CONNECTION_ID) {}
};

struct ServerNewConnectionAck : public Packet
{
    ServerNewConnectionAck() : Packet(SERVER_NEW_CONNECTION_ACK_ID) {}
    FrameNum frameNum;
};

struct ServerWorldUpdateAck : public Packet
{
    ServerWorldUpdateAck() : Packet(SERVER_WORLD_UPDATE_ACK_ID) {}
    FrameNum frameNum;
};

struct ServerInputPacket : public Packet
{
    ServerInputPacket() : Packet(SERVER_INPUT_PACKET_ID) {}
    int value;
};
