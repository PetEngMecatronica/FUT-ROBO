#ifndef TEAM_HPP
#define TEAM_HPP

#include "futbot/Color.hpp"
// #include "futbot/Calibration.hpp"
// #include "futbot/Publisher.hpp"

class Team {
public:
    enum class MatchSide : uint8_t {
        HOME,
        AWAY,
    };

    struct Player {
        Player(const Color& color) : color{color} {}

        std::string mqttTopic;
        const Color& color;
        cv::Point2f centroidCircle;
        cv::Point2f centroidRect;
        float radiusCircle;
        bool found;
    };

    Team(char mqttTopicPrefix, const Color& teamColor, const Color& player1Color,
        const Color& player2Color, const std::string& configFile);
    void readFile(const std::string& filename);
    const std::array<Team::Player, 2>& findPoses(Video& video);
    // void publish_poses(Publisher& pub, Team::MatchSide match_side);
    // inline std::string homeOrAway(Team::MatchSide matchSide) const;
    const std::array<Team::Player, 2>& players() const { return m_players; }
    // void invertThetaAngles();

private:
    // struct ROI {
    //     cv::Mat frameHsv;
    //     cv::Point pt1;
    //     cv::Point pt2;
    // };

    cv::Mat getRoi(const cv::Mat& frame, const cv::Point& centroid);
    // void find_player(Team::Player& player, cv::Point& centroid, Video& video);
    // double getTheta(const cv::Point& rectPoint, const cv::Point& circlePoint);
    bool allPlayersFound() const;

    std::string m_name;
    const Color& m_teamColor;
    // const Calibration& m_calib;
    std::array<Player, 2> m_players;
    double m_rectMinArea;
    double m_circleMinArea;
    cv::Rect m_roiRect;
    // Team::MatchSide m_matchSide;
};

#endif // TEAM_HPP
