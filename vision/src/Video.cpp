#include "futbot/Video.hpp"

#include <format>
#include <print>
#include <stdexcept>
#include <tuple>
#include <opencv2/imgproc.hpp>

static auto readConfig(const std::string& configFile)
{
    cv::FileStorage fs(configFile, cv::FileStorage::READ);

    if (!fs.isOpened()) {
        auto errorMsg = std::format("Couldn't open file `{}`", configFile);
        throw std::runtime_error(errorMsg);
    }

    auto cameraId     = static_cast<int>(fs["camera"]["id"]);
    auto cameraWidth  = static_cast<double>(fs["camera"]["width"]);
    auto cameraHeight = static_cast<double>(fs["camera"]["height"]);
    auto cameraFps    = static_cast<double>(fs["camera"]["fps"]);
    auto windowName   = static_cast<std::string>(fs["window"]["name"]);
    auto windowDelay  = static_cast<int>(fs["window"]["delay"]);

    fs.release();

    return std::make_tuple(cameraId, cameraWidth, cameraHeight, cameraFps,
        windowName, windowDelay);
}

Video::Video(const std::string& configFile)
{
    // Get configuration parameters from file
    auto [cameraId, cameraWidth, cameraHeight, cameraFps,
        windowName, windowDelay] = readConfig(configFile);

    // Open the camera
    m_cap.open("test/output.avi"); // rascunho
    // m_cap.open(cameraId);
    if (!m_cap.isOpened()) {
        auto errorMsg = std::format("Couldn't open camera `{}`", cameraId);
        throw std::runtime_error(errorMsg);
    }

    // Set camera parameters
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, cameraWidth);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, cameraHeight);
    m_cap.set(cv::CAP_PROP_FPS, cameraFps);

    // Set window parameters
    m_windowName = windowName;
    m_windowDelay = windowDelay;

    // Create window
    cv::namedWindow(m_windowName);
}

Video::~Video()
{
    m_cap.release();
    cv::destroyWindow(m_windowName);
}

void Video::updateFrame()
{
    // Get the frame from video capture
    m_cap >> m_frame;

#ifdef MY_DEBUG
    if (m_frame.empty()) {
        // Rewind video if `cameraId` was a path to a video file
        std::println("Rewinding video...");
        m_cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        m_cap >> m_frame;
    }
#endif

    // Create a copy of the frame in HSV color space
    cv::cvtColor(m_frame, m_frameHsv, cv::COLOR_BGR2HSV);
}

int Video::showFrame() const
{
    cv::imshow(m_windowName, m_frame);
    return cv::waitKey(m_windowDelay);
}

int Video::showMaskedFrameHsv(const cv::Scalar& lowerb,
    const cv::Scalar& upperb) const
{
    // Create mask with lower and upper bounds
    cv::Mat mask;
    cv::inRange(m_frameHsv, lowerb, upperb, mask);

    // Create an masked frame using the created mask
    cv::Mat frameMasked;
    m_frame.copyTo(frameMasked, mask);

    // Show frame using color mask
    cv::imshow(m_windowName, frameMasked);
    return cv::waitKey(m_windowDelay);
}

void Video::putText(const cv::String& text, const cv::Point& org,
    const cv::Scalar& color)
{
    cv::putText(m_frame, text, org, cv::FONT_HERSHEY_PLAIN, 1.5, color, 2);
}

void Video::drawCircle(const cv::Point& center, int radius, const cv::Scalar& color)
{
    cv::circle(m_frame, center, radius, color, 2);
    // if (!text.empty()) {
    //     cv::putText(m_frame, text, center, fontName, 0.8, cv::Scalar(0, 0, 0));
    // }
}

void Video::drawContour(const std::vector<cv::Point>& contour, const cv::Scalar& color)
{
    cv::drawContours(m_frame, contour, -1, color);
}

void Video::drawRect(const cv::Rect& rect, const cv::Scalar& color)
{
    cv::rectangle(m_frame, rect, color, 2);
}

void Video::drawPolyline(const std::vector<cv::Point>& pts, bool isClosed,
    const cv::Scalar& color)
{
    cv::polylines(m_frame, pts, isClosed, color);
}
