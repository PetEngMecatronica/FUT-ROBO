#include "futbot/Calibration.hpp"
#include <format>
#include <print>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

constexpr std::string CALIB_CONFIG_FILE = "data/calib.yaml";

static const std::vector<cv::Point> XY_POINTS = {
    {0, 0},
    {-1190, 800},
    {0, 800},
    {1190, 800},
    {1190, -800},
    {0, -800},
    {-1190, -800}
};

void Calibration::calibrate(Video& video)
{
    cv::FileStorage fs;

    // Try to read homography matrix from file
    fs.open(CALIB_CONFIG_FILE, cv::FileStorage::READ);
    if (fs.isOpened()) {
        fs["homographyMatrix"] >> m_homographyMatrix;
        fs.release();
        return;
    }

    std::vector<cv::Point> uvPoints;

    // Sets mouse callback function
    cv::setMouseCallback(
        video.windowName(),
        [](int event, int x, int y, int flags, void* userdata) -> void {
            (void)flags; // Prevent unused parameter warning
            auto& uvPointsRef =
                *static_cast<std::vector<cv::Point>*>(userdata);

            if (event == cv::EVENT_LBUTTONDOWN) {
                uvPointsRef.emplace_back(x, y);
                std::println("Ponto {} selecionado: {}, {}", uvPointsRef.size(), x, y); // rascunho
            }
        },
        &uvPoints
    );

    // Loop until user selects all points
    while (uvPoints.size() < XY_POINTS.size()) {
        video.updateFrame();
        video.putText(std::format("Select the {} point", uvPoints.size() + 1));
        int key = video.showFrame();
        if (key == 27) { // key == ESC
            exit(EXIT_SUCCESS);
        }
    }

    // Unsets mouse callback function
    cv::setMouseCallback(video.windowName(), nullptr);

    // Create the homography Matrix
    m_homographyMatrix = cv::findHomography(uvPoints, XY_POINTS);

    // Save homography matrix in a file
    fs.open(CALIB_CONFIG_FILE, cv::FileStorage::WRITE);
    fs << "homographyMatrix" << m_homographyMatrix;
    fs.release();
}

cv::Point2f Calibration::uvToXy(const cv::Point2f& uv) const
{
    // Although we only want to transform a single point, the
    // `cv::perspectiveTransform` function only accepts vectors of points
    std::vector<cv::Point2f> src = { uv };
    std::vector<cv::Point2f> dst;

    cv::perspectiveTransform(src, dst, m_homographyMatrix);

    return dst.front();
}
