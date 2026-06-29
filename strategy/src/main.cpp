#include <algorithm>
#include <cmath>
#include <print>
#include "common/protocol.h"
#include "strategy/Listener.hpp"

constexpr float RAD2DEG = 180.0f / M_PIf;

constexpr char MULTICAST_ADDRESS[] = "224.1.1.1";
constexpr int MULTICAST_PORT = 5007;

constexpr float KP = 0.5f;

TwistCmd compute_twist_control(RobotPose robot, Vec2 ball, float kp) {
    TwistCmd cmd = {0.0f, 0.0f};

    float dx = static_cast<float>(ball.x - robot.position.x);
    float dy = static_cast<float>(ball.y - robot.position.y);

    // Calculate heading error and wrap to [-pi, pi]
    float target_angle = std::atan2(dy, dx);
    float error_theta = target_angle - robot.angle;
    error_theta = std::remainder(error_theta, 2.0f * M_PIf);

    // Define velocity constraints
    constexpr float max_angular_vel = 2.0f; // rad/s

    // The robot will only rotate
    cmd.v = 0.0f;

    // Cap angular velocity to prevent violent snaps
    cmd.w = std::clamp(kp * error_theta, -max_angular_vel, max_angular_vel);

    return cmd;
}

int main() {
    try {
        std::println("Starting Strategy Listener Node...");
        Listener listener(MULTICAST_ADDRESS, MULTICAST_PORT);

        // Allocate a local struct buffer to receive the raw bytes directly into
        FieldPoses field{};
        size_t expected_size = sizeof(struct FieldPoses);

        // Reinterpret cast the struct memory block to a byte pointer
        auto buffer = reinterpret_cast<uint8_t*>(&field);

        while (true) {
            size_t bytes_read = listener.receive(buffer, expected_size);

            if (bytes_read == expected_size) {
                // Instantly ready to use with zero deserialization overhead!
                std::println("--- New Frame Received ---");
                std::println("Ball: ({}, {})", field.ball.x, field.ball.y);

                for (int i = 0; i < 4; i++) {
                    std::println("Robot ID {}: Pos({}, {}) Angle: {}°",
                        field.robots[i].id,
                        field.robots[i].position.x,
                        field.robots[i].position.y,
                        field.robots[i].angle * RAD2DEG
                    );
                }
            } else if (bytes_read > 0) {
                std::println("Warning: Received packet size mismatch (Got {} bytes, expected {})",
                             bytes_read, expected_size);
            }
        }
    } catch (const std::exception& e) {
        std::println("Exception in listener loop: {}", e.what());
        return 1;
    }

    return 0;
}
