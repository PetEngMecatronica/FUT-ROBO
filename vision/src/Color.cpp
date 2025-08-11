#include "futbot/Color.hpp"

#include <format>
#include <opencv2/imgproc.hpp>
#include <optional>

Color::Color(const std::string& name, const std::string& configFile) : m_name{name}
{
    cv::FileStorage fs(configFile, cv::FileStorage::READ);

    fs["color"]["hue_tol"] >> m_hueTolerance;
    fs["color"]["sat_tol"] >> m_satTolerance;

    fs.release();
}

Color::~Color()
{
}

bool Color::readFile()
{
    cv::FileStorage fs(std::format("data/{}.yaml", m_name), cv::FileStorage::READ);

    if (fs.isOpened()) {
        fs["lowerb"] >> m_lowerb;
        fs["upperb"] >> m_upperb;

        fs.release();

        return true;
    }

    return false;
}

bool Color::writeFile()
{
    cv::FileStorage fs(std::format("data/{}.yaml", m_name), cv::FileStorage::WRITE);

    if (fs.isOpened()) {
        fs << "lowerb" << m_lowerb;
        fs << "upperb" << m_upperb;

        fs.release();

        return true;
    }

    return false;
}

static cv::Vec3b clickColor(Video& video, const std::string& colorName)
{
    std::optional<cv::Point> clickPoint = std::nullopt;

    // Sets mouse callback function
    cv::setMouseCallback(
        video.windowName(),
        [](int event, int x, int y, int flags, void* userdata) -> void {
            (void)flags; // Prevent unused parameter warning

            if (event == cv::EVENT_LBUTTONDOWN) {
                auto& clickPointRef =
                    *static_cast<std::optional<cv::Point>*>(userdata);
                clickPointRef = cv::Point(x, y);
            }
        },
        &clickPoint
    );

    // Loop until the user select a color
    do {
        video.updateFrame();
        video.putText(std::format("Selecione a cor `{}`", colorName));
        int key = video.showFrame();
        if (key == 27) { // key == ESC
            exit(EXIT_SUCCESS);
        }
    } while (!clickPoint.has_value());

    // Unsets mouse callback function
    cv::setMouseCallback(video.windowName(), nullptr);

    // Return the selected color
    return video.frameHsv().at<cv::Vec3b>(clickPoint->y, clickPoint->x);
}

void Color::select(Video& video)
{
    cv::Vec3b clickHsv = clickColor(video, m_name);

    // Store the selected hsv color
    m_lowerb = cv::Scalar(
        std::clamp(static_cast<int>(clickHsv[0]) - m_hueTolerance, 0, 179),
        std::clamp(static_cast<int>(clickHsv[1]) - m_satTolerance, 0, 255),
        20
    );
    m_upperb = cv::Scalar(
        std::clamp(static_cast<int>(clickHsv[0]) + m_hueTolerance, 0, 179),
        std::clamp(static_cast<int>(clickHsv[1]) + m_satTolerance, 0, 255),
        255
    );

    writeFile();
    showColorOnVideo(video);
}

void Color::showColorOnVideo(Video& video) const
{
    int key;

    do {
        video.updateFrame();
        key = video.showMaskedFrameHsv(m_lowerb, m_upperb);
    } while (key == -1); // while none key is pressed
}

void Color::findContours(const cv::Mat& frameHsv,
    std::vector<std::vector<cv::Point>> &contours) const
{
    cv::Mat mask;
    cv::inRange(frameHsv, m_lowerb, m_upperb, mask);

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
}

std::vector<std::vector<cv::Point>> Color::findNLargestContours(
    const cv::Mat& frameHsv, int n) const
{
    std::vector<std::vector<cv::Point>> contours;
    findContours(frameHsv, contours);

    std::nth_element(
        contours.begin(),
        contours.begin() + n,
        contours.end(),
        [](const auto& a, const auto& b) {
            return cv::contourArea(a) > cv::contourArea(b);
        }
    );

    contours.resize(n);

    return contours;
}

std::vector<cv::Point> Color::findLargestContour(const cv::Mat& frameHsv) const
{
    std::vector<std::vector<cv::Point>> contours;
    findContours(frameHsv, contours);

    // Return an empty contour if `contours` is empty
    if (contours.empty()) {
        return std::vector<cv::Point>();
    }

    // Find contour of maximum area
    return *std::max_element(contours.cbegin(), contours.cend(),
        [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        }
    );
}
