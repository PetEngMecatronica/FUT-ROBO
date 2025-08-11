#ifndef ROI_HPP
#define ROI_HPP

#include <opencv2/core.hpp>
#include "futbot/Video.hpp"

class Roi {
public:
    Roi(int width, int height);
    virtual ~Roi();

    cv::Mat getFrameRoi(const cv::Mat& frame, const cv::Point& centroid);

    // Getters for member variables
    const cv::Rect& rect() const { return m_rect; }

private:
    cv::Rect m_rect;
};

#endif // ROI_HPP
