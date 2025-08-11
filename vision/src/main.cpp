#include <cstdlib>
#include <format>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <print>
#include <string>
#include <cmath>
#include "futbot/Video.hpp"
#include "futbot/Calibration.hpp"
#include "futbot/Team.hpp"
// #include "futbot/Publisher.hpp"
#include "futbot/Ball.hpp"

constexpr double RAD2DEG = 180 / M_PI;
constexpr std::string CONFIG_FILE = "config.yaml";

static inline float getTheta(const cv::Point2f pt1, const cv::Point2f pt2)
{
    return std::atan2(pt1.y - pt2.y, pt1.x - pt2.x) * RAD2DEG;
}

int main()
{
    // Publisher publisher("MANAGER", "tcp://localhost:1883");
    Video video(CONFIG_FILE);
    Color orange("laranja", CONFIG_FILE), green("verde", CONFIG_FILE),
        blue("azul", CONFIG_FILE), pink("rosa", CONFIG_FILE),
        yellow("amarelo", CONFIG_FILE);
    Calibration calib;
    Ball ball(orange);
    std::array<Team, 2> teams{
        Team{'H', green, pink, yellow, CONFIG_FILE},
        Team{'A', blue, pink, yellow, CONFIG_FILE}
    };

    try {
        int key;

        calib.calibrate(video);

        if (!orange.readFile()) orange.select(video);
        if (!green.readFile()) green.select(video);
        if (!blue.readFile()) blue.select(video);
        if (!pink.readFile()) pink.select(video);
        if (!yellow.readFile()) yellow.select(video);

        do {
            video.updateFrame();

            // Find the ball pose
            auto ballResult = ball.findCentroid(video.frameHsv());
            if (ballResult.has_value()) {
                const auto& [ballCentroid, ballRadius] = ballResult.value();
                cv::Point2f ballRealCentroid = calib.uvToXy(ballCentroid);
                std::string ballMsg = std::format("{:.0f},{:.0f}",
                    ballRealCentroid.x, ballRealCentroid.y);
                // std::println("ball radius: {}", ballRadius); // rascunho
                // publisher.publish(ballMsg, "BALL");
#ifdef MY_DEBUG
                video.drawCircle(ballCentroid, ballRadius, Color::CYAN);
                video.putText(ballMsg, ballCentroid);
#endif // MY_DEBUG
            } else {
                std::println("Ball wasn't found!");
            }

            // Find the four robots; there's two teams, and each team has two robots
            for (auto& team : teams) {
                team.findPoses(video);
                for (const auto& player : team.players()) {
                    cv::Point2f playerRealRectCentroid = calib.uvToXy(player.centroidRect);
                    cv::Point2f playerRealCircleCentroid = calib.uvToXy(player.centroidCircle);
                    float playerAngle = getTheta(playerRealCircleCentroid, playerRealRectCentroid);
                    std::string playerMsg = std::format("{:.0f},{:.0f},{:.0f}",
                        playerRealRectCentroid.x, playerRealRectCentroid.y, playerAngle);
                    // publisher.publish(playerMsg, player.mqttTopic);
#ifdef MY_DEBUG
                    video.putText(playerMsg, player.centroidRect);
#endif // MY_DEBUG
                }
            }

            key = video.showFrame();
        } while (key != 27);
    } catch (const std::exception& e) {
        std::println(std::cerr, "Error: {}", e.what());
        return 1;
    }

    return 0;
}
