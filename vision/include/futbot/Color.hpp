#ifndef COLOR_HPP
#define COLOR_HPP

#include <opencv2/core.hpp>
#include <vector>
#include "futbot/Video.hpp"

class Color {
public:
    // Color BGR values
    inline static auto BLACK = cv::Scalar(0, 0, 0);
    inline static auto WHITE = cv::Scalar(255, 255, 255);
    inline static auto BLUE = cv::Scalar(255, 0, 0);
    inline static auto GREEN = cv::Scalar(0, 255, 0);
    inline static auto RED = cv::Scalar(0, 0, 255);
    inline static auto YELLOW = cv::Scalar(0, 255, 255);
    inline static auto CYAN = cv::Scalar(255, 255, 0);
    inline static auto PINK = cv::Scalar(255, 0, 255);

    // Constructor and destructor
    Color(const std::string& name, const std::string& configFile);
    virtual ~Color();

    // Read and write color params on a file
    bool readFile();
    bool writeFile();

    // Select color from a video stream
    void select(Video& video);
    void showColorOnVideo(Video& video) const;

    // Get color contours on a frame
    void findContours(const cv::Mat& frameHsv,
        std::vector<std::vector<cv::Point>> &contours) const;
    std::vector<std::vector<cv::Point>> findNLargestContours(
        const cv::Mat& frameHsv, int n) const;
    std::vector<cv::Point> findLargestContour(const cv::Mat& frameHsv) const;

    // Getters for member variables
    const std::string& name() const { return m_name; }
    const cv::Scalar& lowerb() const { return m_lowerb; }
    const cv::Scalar& upperb() const { return m_upperb; }

private:
    // Color name
    std::string m_name;

    // Lower and upper bounds of the HSV color
    cv::Scalar m_lowerb;
    cv::Scalar m_upperb;

    // HUE and saturation tolerance to select the colors
    int m_hueTolerance;
    int m_satTolerance;
};

#endif // COLOR_HPP
