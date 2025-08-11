#include "futbot/Roi.hpp"

Roi::Roi(int width, int height) : m_rect{0, 0, width, height}
{
}

Roi::~Roi()
{
}

cv::Mat Roi::getFrameRoi(const cv::Mat& frame, const cv::Point& centroid)
{
    const int maxX = frame.cols - m_rect.width;
    const int maxY = frame.rows - m_rect.height;
    cv::Mat frameRoi;

    m_rect.x = std::clamp(centroid.x, 0, maxX);
    m_rect.y = std::clamp(centroid.y, 0, maxY);

    frameRoi = frame(m_rect);

    return frameRoi;
}
