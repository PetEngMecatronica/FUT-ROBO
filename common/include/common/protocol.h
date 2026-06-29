#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Force 1-byte packing across both C and C++ compilers
#pragma pack(push, 1)

struct Vec2 {
    int16_t x;
    int16_t y;
};

struct RobotPose {
    uint8_t id;
    struct Vec2 position;
    float angle;
};

struct FieldPoses {
    struct Vec2 ball;
    struct RobotPose robots[4];
};

struct TwistCmd {
    float v; // m/s
    float w; // rad/s
};

#pragma pack(pop) // Restore default compiler alignment settings

#endif // PROTOCOL_H
