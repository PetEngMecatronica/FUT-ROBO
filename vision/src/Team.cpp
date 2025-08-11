#include "futbot/Team.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <opencv2/core/types.hpp>
#include <print>
#include <opencv2/imgproc.hpp>

Team::Team(char mqttTopicPrefix, const Color& teamColor, const Color& player1Color,
    const Color& player2Color, const std::string& configFile)
    : m_teamColor{teamColor}, m_players{player1Color, player2Color}
{
    for (auto& player : m_players) {
        player.mqttTopic = std::format("{}{}{}", mqttTopicPrefix,
            m_teamColor.name(), player.color.name());
    }

    readFile(configFile);
}

void Team::readFile(const std::string& filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);

    if (!fs.isOpened()) {
        auto errorMsg = std::format("Couldn't read file `{}`", filename);
        throw std::runtime_error(errorMsg);
    }

    fs["color"]["rectMinArea"] >> m_rectMinArea;
    fs["color"]["circleMinArea"] >> m_circleMinArea;
    fs["color"]["roiWidth"] >> m_roiRect.width;
    fs["color"]["roiHeight"] >> m_roiRect.height;

    fs.release();
}

static std::vector<cv::Point> RotatedRectPoints(const cv::RotatedRect& rotatedRect)
{
    std::vector<cv::Point> rectPts;
    cv::Point2f pts[4];

    rotatedRect.points(pts);

    for (int i = 0; i < 4; i++) {
        rectPts.emplace_back(pts[i].x, pts[i].y);
    }

    return rectPts;
}

cv::Mat Team::getRoi(const cv::Mat& frame, const cv::Point& centroid)
{
    const int topLeftX = centroid.x - m_roiRect.width / 2;
    const int topLefty = centroid.y - m_roiRect.height / 2;
    const int maxX = frame.cols - m_roiRect.width;
    const int maxY = frame.rows - m_roiRect.height;

    m_roiRect.x = std::clamp(topLeftX, 0, maxX);
    m_roiRect.y = std::clamp(topLefty, 0, maxY);

    cv::Mat frameHsvRoi = frame(m_roiRect);

    return frameHsvRoi;
}

const std::array<Team::Player, 2>& Team::findPoses(Video& video)
{
    std::vector<std::vector<cv::Point>> contours;
    m_teamColor.findContours(video.frameHsv(), contours);

    // Set players as not found
    for (auto& player : m_players) {
        player.found = false;
    }

    // std::println("centroids size: {}", centroids.size()); // rascunho

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) < m_rectMinArea) continue;

        cv::RotatedRect contourRect = cv::minAreaRect(contour);
        std::vector<cv::Point> rotatedRectPts = RotatedRectPoints(contourRect);
        cv::Mat frameRoi = getRoi(video.frameHsv(), contourRect.center);

        for (auto& player : m_players) {
            if (player.found) continue;

            std::vector<cv::Point> circleContourRoi =
                player.color.findLargestContour(frameRoi);
            if (circleContourRoi.empty()) continue;
            double circleContourRoiArea = cv::contourArea(circleContourRoi);

            // Continue if circle with minimum area wasn't found
            if (circleContourRoi.empty()
            || circleContourRoiArea < m_circleMinArea) continue;

            // TODO: o centro do circulo está em relação ao ROI
            cv::Point2f circleCentroidRoi;
            float radiusCircle;
            cv::minEnclosingCircle(circleContourRoi, circleCentroidRoi, radiusCircle);
            // std::println("robot cicle radius: {}", radiusCircle); // rascunho

            player.centroidCircle.x = circleCentroidRoi.x + m_roiRect.x;
            player.centroidCircle.y = circleCentroidRoi.y + m_roiRect.y;
            player.centroidRect = contourRect.center;

#ifdef MY_DEBUG
            video.drawCircle(player.centroidCircle, player.radiusCircle, Color::PINK);
            video.drawCircle(player.centroidRect, 3, Color::YELLOW);
            video.drawRect(m_roiRect, Color::BLUE);
            video.drawPolyline(rotatedRectPts, true, Color::GREEN);
#endif // MY_DEBUG

            player.found = true;
            // std::println("Found player: {}|{}", m_teamColor.name(), player.color.name()); // rascunho
        }

        // Break loop if all players were found
        if (std::all_of(m_players.cbegin(), m_players.cend(),
            [](const Player& p) { return p.found; }
        )) {
            // std::println("All found!"); // rascunho
            break;
        }
    }

    return m_players;
}

// void Team::publish_poses(Publisher& pub, Team::MatchSide match_side)
// {
//     constexpr double RAD2DEG = 180 / M_PI;

//     // auto pub_topic = [this](const Team::Player& p, Team::MatchSide match_side) -> std::string {
//     //     const std::string home_or_away =
//     //         match_side == MatchSide::HOME ? "HOME" :
//     //         match_side == MatchSide::AWAY ? "AWAY" :
//     //         "UNKNOWN";
//     //     const char team_char = std::toupper(this->team_color.name()[0]);
//     //     const char player_char = std::toupper(p.color.name()[0]);

//     //     return home_or_away + '-' + team_char + player_char;
//     // };

//     for (const auto& p : players_) {
//         const std::string pub_msg =
//             std::to_string(p.centroid_rect_world.x) + ',' +
//             std::to_string(p.centroid_rect_world.y) + ',' +
//             std::to_string(static_cast<int>(p.theta * RAD2DEG)) + ',';
//         const std::string pub_topic = home_or_away(match_side) + '-' +
//             static_cast<char>(std::toupper(this->team_color.name()[0])) +
//             static_cast<char>(std::toupper(p.color.name()[0]));

//         // std::cout << "Publishing in topic " << pub_topic << std::endl; // rascunho
//         pub.publish(pub_msg, pub_topic);
//     }
// }

// inline std::string Team::homeOrAway(Team::MatchSide matchSide) const
// {
//     return matchSide == MatchSide::HOME ? "HOME" :
//            matchSide == MatchSide::AWAY ? "AWAY" :
//            "UNKNOWN";
// }

// double Team::getTheta(const cv::Point& rectPoint, const cv::Point& circlePoint)
// {
//     int deltaX = circlePoint.y - rectPoint.y;
//     int deltaY = circlePoint.x - rectPoint.x;
//     // std::cout << "deltaX: " << deltaX << '\n'  // rascunho
//     //           << "deltaY: " << deltaY << '\n'; // rascunho
//     return std::atan2(deltaX, deltaY);
// }
