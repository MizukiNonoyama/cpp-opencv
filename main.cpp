#include <opencv2/opencv.hpp>
#include <fmt/color.h>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

namespace {

// "Parses" an image file: reads its raw bytes and decodes them into pixel
// data, mirroring what a codec does under the hood (as opposed to just
// calling cv::imread, which hides both steps behind one call).
cv::Mat parseImage(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open '" + path.string() + "'");
    }
    const std::vector<uchar> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    cv::Mat img = cv::imdecode(raw, cv::IMREAD_COLOR);
    if (img.empty()) {
        throw std::runtime_error("Failed to parse image data from '" + path.string() + "'");
    }
    return img;
}

// Reverses parseImage: re-encodes pixel data back into a file format's
// byte representation (e.g. JPEG). ext is a file extension like ".jpg".
std::vector<uchar> restoreImage(const cv::Mat& img, const std::string& ext)
{
    std::vector<uchar> encoded;
    if (!cv::imencode(ext, img, encoded)) {
        throw std::runtime_error("Failed to restore image data for extension '" + ext + "'");
    }
    return encoded;
}

// Writes encoded image bytes to disk, creating the destination directory
// if needed.
void saveImage(const std::vector<uchar>& data, const fs::path& outPath)
{
    fs::create_directories(outPath.parent_path());

    std::ofstream file(outPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open '" + outPath.string() + "' for writing");
    }
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

} // namespace

int main(int argc, char** argv)
{
    fmt::print(fmt::emphasis::bold | fg(fmt::color::cyan), "OpenCV {}\n", CV_VERSION);

    const fs::path inputPath = argc > 1 ? fs::path(argv[1]) : fs::path("C:/opencv/sources/samples/data/fruits.jpg");

    cv::Mat src;
    try {
        src = parseImage(inputPath);
    } catch (const std::exception& e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print("Loaded '{}' ({}x{})\n", inputPath.string(), src.cols, src.rows);

    const cv::Vec3b pixel00 = src.at<cv::Vec3b>(0, 0);
    fmt::print("Pixel (0,0): B={} G={} R={}\n", pixel00[0], pixel00[1], pixel00[2]);
    // 以上 claude codeによる

    cv::Mat grayScale = src.clone();
    for (int i = 1; i < src.cols - 1; ++i) {
        for (int j = 1; j < src.rows - 1; ++j) {
            const cv::Vec3b pixel = src.at<cv::Vec3b>(i, j);
            int avg = (pixel[0] + pixel[1] + pixel[2]) / 3;
            cv::Vec3b newPixel;
            newPixel[0] = avg;
            newPixel[1] = avg;
            newPixel[2] = avg;
            grayScale.at<cv::Vec3b>(i, j) = newPixel;
        }
    }
    cv::Mat gray, blurred, edges;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blurred, {5, 5}, 1.4);
    cv::Canny(blurred, edges, 60, 160);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat annotated = src.clone();
    cv::drawContours(annotated, contours, -1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    fmt::print("Detected {} contours\n", contours.size());

    // 以下 claude code による
    const fs::path outputPath = "./output/fruit_processed.jpg";
    try {
        const std::vector<uchar> restored = restoreImage(annotated, outputPath.extension().string());
        saveImage(restored, outputPath);
    } catch (const std::exception& e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print(fg(fmt::color::green), "Wrote result to '{}'\n", outputPath.string());

    /*
    try {
        cv::imshow("Original", src);
        cv::imshow("Edges", edges);
        cv::imshow("Contours", annotated);
        fmt::print("Press any key in an image window to exit...\n");
        cv::waitKey(0);
    } catch (const cv::Exception& e) {
        fmt::print(fg(fmt::color::yellow),
                   "GUI display unavailable ({}); result was still saved to disk.\n", e.what());
    }
    */

    return 0;
}
