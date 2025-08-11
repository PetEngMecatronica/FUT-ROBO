#include "futbot/Ball.hpp"

#include <opencv2/imgproc.hpp>

Ball::Ball(const Color& color) : m_color{color}
{
}

Ball::~Ball()
{
}

std::optional<std::tuple<cv::Point2f, float>> Ball::findCentroid(
    const cv::Mat& frame) const
{
    const auto contour = m_color.findLargestContour(frame);

    if (contour.empty()) {
        return std::nullopt;
    }

    // Find circle centroid
    cv::Point2f centroid;
    float radius;
    cv::minEnclosingCircle(contour, centroid, radius);

    // Return tuple with circle centroid and radius. To get values, use:
    // auto [centroid, radius] = ball.findCentroid(frame);
    return std::make_tuple(centroid, radius);
}
