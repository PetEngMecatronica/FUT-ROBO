#ifndef CALIBRATION_HPP
#define CALIBRATION_HPP

#include <opencv2/core.hpp>
#include "futbot/Video.hpp"

class Calibration {
public:
    void calibrate(Video& video);
    cv::Point2f uvToXy(const cv::Point2f& uv) const;

private:
    void fileRead();
    void fileWrite();

    cv::Mat m_homographyMatrix;
};

#endif // CALIBRATION_HPP
