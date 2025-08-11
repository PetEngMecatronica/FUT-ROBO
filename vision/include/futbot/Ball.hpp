#ifndef BALL_HPP
#define BALL_HPP

#include <opencv2/core.hpp>
#include "futbot/Color.hpp"

class Ball {
public:
    Ball(const Color& color);
    virtual ~Ball();

    std::optional<std::tuple<cv::Point2f, float>> findCentroid(
        const cv::Mat& frame) const;

private:
    const Color& m_color;
};

#endif // BALL_HPP
