
#include <opencv2/opencv.hpp>
#include <fmt/color.h>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <numbers>

namespace fs = std::filesystem;

namespace {
    // ########################################################################################################################
    // ########################################### START CLAUDE CODE ##########################################################
    // ########################################################################################################################

    // "Parses" an image file: reads its raw bytes and decodes them into pixel
    // data, mirroring what a codec does under the hood (as opposed to just
    // calling cv::imread, which hides both steps behind one call).
    cv::Mat parseImage(const fs::path &path) {
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

    // Reads a video file and decodes it into a sequence of per-frame images,
    // mirroring parseImage but for video sources. VideoCapture reuses the
    // same internal buffer across reads, so each frame is cloned before
    // being stored.
    std::vector<cv::Mat> parseVideoFrames(const fs::path &path) {
        cv::VideoCapture cap(path.string());
        if (!cap.isOpened()) {
            throw std::runtime_error("Failed to open '" + path.string() + "'");
        }

        std::vector<cv::Mat> frames;
        cv::Mat frame;
        while (cap.read(frame)) {
            frames.push_back(frame.clone());
        }

        if (frames.empty()) {
            throw std::runtime_error("Failed to parse video data from '" + path.string() + "'");
        }
        return frames;
    }

    // Reverses parseImage: re-encodes pixel data back into a file format's
    // byte representation (e.g. JPEG). ext is a file extension like ".jpg".
    std::vector<uchar> restoreImage(const cv::Mat &img, const std::string &ext) {
        std::vector<uchar> encoded;
        if (!cv::imencode(ext, img, encoded)) {
            throw std::runtime_error("Failed to restore image data for extension '" + ext + "'");
        }
        return encoded;
    }

    // Writes encoded image bytes to disk, creating the destination directory
    // if needed.
    void saveImage(const std::vector<uchar> &data, const fs::path &outPath) {
        fs::create_directories(outPath.parent_path());

        std::ofstream file(outPath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open '" + outPath.string() + "' for writing");
        }
        file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    int outputImage(const cv::Mat &img, const std::string &ext) {
        const fs::path outputPath = ext;
        try {
            const std::vector<uchar> restored = restoreImage(img, outputPath.extension().string());
            saveImage(restored, outputPath);
        } catch (const std::exception &e) {
            fmt::print(fg(fmt::color::red), "{}\n", e.what());
            return 1;
        }
        fmt::print(fg(fmt::color::green), "Wrote result to '{}'\n", outputPath.string());
        return 0;
    }
} // namespace

int main(int argc, char **argv) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::cyan), "OpenCV {}\n", CV_VERSION);

    const fs::path inputPath = argc > 1 ? fs::path(argv[1]) : fs::path("C:/opencv/sources/samples/data/fruits.jpg");

    cv::Mat src;
    try {
        src = parseImage(inputPath);
    } catch (const std::exception &e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print("Loaded '{}' ({}x{})\n", inputPath.string(), src.cols, src.rows);

    // ########################################################################################################################
    // ############################################# END CLAUDE CODE ##########################################################
    // ########################################################################################################################

    const fs::path inputPathAloeL = argc > 1 ? fs::path(argv[1]) : fs::path("C:/opencv/sources/samples/data/aloeL.jpg");
    cv::Mat aloeL;
    try {
        aloeL = parseImage(inputPathAloeL);
    } catch (const std::exception &e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print("Loaded '{}' ({}x{})\n", inputPathAloeL.string(), aloeL.cols, aloeL.rows);
    const fs::path inputPathAloeR = argc > 1 ? fs::path(argv[1]) : fs::path("C:/opencv/sources/samples/data/aloeR.jpg");
    cv::Mat aloeR;
    try {
        aloeR = parseImage(inputPathAloeR);
    } catch (const std::exception &e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print("Loaded '{}' ({}x{})\n", inputPathAloeR.string(), aloeR.cols, aloeR.rows);

    const fs::path inputPathRoboCup = "./robo_cup_incheon.mp4";
    std::vector<cv::Mat> roboCupFrames;
    try {
        roboCupFrames = parseVideoFrames(inputPathRoboCup);
    } catch (const std::exception &e) {
        fmt::print(fg(fmt::color::red), "{}\n", e.what());
        return 1;
    }
    fmt::print("Loaded '{}' ({} frames, {}x{})\n", inputPathRoboCup.string(), roboCupFrames.size(),
               roboCupFrames.front().cols, roboCupFrames.front().rows);

    int errorCode = 0;
    errorCode += outputImage(src, "./output/raw.jpg");
    errorCode += outputImage(aloeL, "./output/aloeL.jpg");
    errorCode += outputImage(aloeR, "./output/aloeR.jpg");

    // make gray scale
    cv::Mat grayScale = src.clone();
    {
        int count = 0;
        for (int i = 0; i < src.rows; i++) {
            for (int j = 0; j < src.cols; j++) {
                const cv::Vec3b pixel = src.at<cv::Vec3b>(i, j);
                count++;
                const int avg = (pixel[0] + pixel[1] + pixel[2]) / 3;
                cv::Vec3b newPixel;
                newPixel[0] = avg;
                newPixel[1] = avg;
                newPixel[2] = avg;
                grayScale.at<cv::Vec3b>(i, j) = newPixel;
            }
        }
        errorCode += outputImage(grayScale, "./output/fruit_processed_gray.jpg");
    }

    // ヒストグラム平坦化
    // 度数が集中している領域を引き延ばす
    // 引き延ばす = トーンカーブの傾きを大きくする
    // https://www.ikegami.co.jp/column/detail/41/
    // トーンカーブの最大値は1
    cv::Mat histogram = src.clone();
    {
        std::vector<int> histgram(256, 0);
        for (int i = 0; i < grayScale.rows; i++) {
            for (int j = 0; j < grayScale.cols; j++) {
                const cv::Vec3b pixel = grayScale.at<cv::Vec3b>(i, j);
                histgram[pixel[0]]++;
            }
        }

        int N = 8;
        for (int i = 0; i < 256 / N; i++) {
            int histgramOutput = 0;
            for (int j = 0; j < N; j++) {
                histgramOutput += histgram[i * N + j];
            }
            fmt::print(fg(fmt::color::cyan), "{}\n", histgramOutput);
        }

        std::vector<int> histgramInt(256, 0);
        int integral = 0;
        for (int i = 0; i < 256; i++) {
            integral += histgram[i];
            histgramInt[i] = integral;
        }

        std::vector<int> histgram2(256, 0);
        for (int i = 0; i < grayScale.rows; i++) {
            for (int j = 0; j < grayScale.cols; j++) {
                const cv::Vec3b pixel = grayScale.at<cv::Vec3b>(i, j);
                int s = pixel[0];
                int convert = static_cast<int>(256 * static_cast<double>(histgramInt[s]) / static_cast<double>(integral));
                histgram2[convert]++;

                // output mat
                cv::Vec3b newPixel;
                newPixel[0] = convert;
                newPixel[1] = convert;
                newPixel[2] = convert;
                histogram.at<cv::Vec3b>(i, j) = newPixel;
            }
        }

        fmt::print("\n");
        int integral2 = 0;
        for (int i = 0; i < 256 / N; i++) {
            int histgramOutput = 0;
            for (int j = 0; j < N; j++) {
                integral2 += histgram2[i * N + j];
                histgramOutput += histgram2[i * N + j];
            }
            fmt::print(fg(fmt::color::cyan), "{}\n", histgramOutput);
        }

        errorCode += outputImage(histogram, "./output/histogram.jpg");
    }

    // フルカラーの場合のヒストグラム平坦化
    cv::Mat histgramAloeL = aloeL.clone();
    {
        std::vector<int> histgramR(256, 0);
        std::vector<int> histgramG(256, 0);
        std::vector<int> histgramB(256, 0);
        for (int i = 0; i < aloeL.rows; i++) {
            for (int j = 0; j < aloeL.cols; j++) {
                const cv::Vec3b pixel = aloeL.at<cv::Vec3b>(i, j);
                histgramB[pixel[0]]++;
                histgramG[pixel[1]]++;
                histgramR[pixel[2]]++;
            }
        }

        std::vector<int> histgramIntR(256, 0);
        std::vector<int> histgramIntG(256, 0);
        std::vector<int> histgramIntB(256, 0);
        int pixels = aloeL.rows * aloeL.cols;
        for (int i = 0; i < 256; i++) {
            histgramIntR[i] = histgramR[i] + (i - 1 < 0 ? 0 : histgramIntR[i - 1]);
            histgramIntG[i] = histgramG[i] + (i - 1 < 0 ? 0 : histgramIntG[i - 1]);
            histgramIntB[i] = histgramB[i] + (i - 1 < 0 ? 0 : histgramIntB[i - 1]);
        }

        for (int i = 0; i < aloeL.rows; i++) {
            for (int j = 0; j < aloeL.cols; j++) {
                const cv::Vec3b pixel = aloeL.at<cv::Vec3b>(i, j);
                int convertB = static_cast<int>(256 * static_cast<double>(histgramIntB[pixel[0]]) / static_cast<double>(pixels));
                int convertG = static_cast<int>(256 * static_cast<double>(histgramIntG[pixel[1]]) / static_cast<double>(pixels));
                int convertR = static_cast<int>(256 * static_cast<double>(histgramIntR[pixel[2]]) / static_cast<double>(pixels));

                cv::Vec3b newPixel;
                newPixel[0] = convertB;
                newPixel[1] = convertG;
                newPixel[2] = convertR;
                histgramAloeL.at<cv::Vec3b>(i, j) = newPixel;
            }
        }

        errorCode += outputImage(histgramAloeL, "./output/histogramAloeL.jpg");
    }

    // ソーベルフィルタ
    cv::Mat sobel = src.clone();
    {
        for (int i = 1; i < grayScale.rows - 1; i++) {
            for (int j = 1; j < grayScale.cols - 1; j++) {
                const int gray_1_1 = grayScale.at<cv::Vec3b>(i - 1, j - 1)[0];
                const int gray0_1 = grayScale.at<cv::Vec3b>(i, j - 1)[0];
                const int gray1_1 = grayScale.at<cv::Vec3b>(i + 1, j - 1)[0];
                const int gray_10 = grayScale.at<cv::Vec3b>(i - 1, j)[0];
                const int gray00 = grayScale.at<cv::Vec3b>(i, j)[0];
                const int gray10 = grayScale.at<cv::Vec3b>(i + 1, j)[0];
                const int gray_11 = grayScale.at<cv::Vec3b>(i - 1, j + 1)[0];
                const int gray01 = grayScale.at<cv::Vec3b>(i, j + 1)[0];
                const int gray11 = grayScale.at<cv::Vec3b>(i + 1, j + 1)[0];
                int newGrayVertical = -1 * gray_1_1 - 2 * gray_10 - 1 * gray_11 + 1 * gray1_1 + 2 * gray10 + 1 * gray11;
                int newGrayHorizontal = -1 * gray_1_1 - 2 * gray0_1 - 1 * gray1_1 + 1 * gray_11 + 2 * gray01 + 1 * gray11;
                int grad = static_cast<int>(std::sqrt(newGrayVertical * newGrayVertical + newGrayHorizontal * newGrayHorizontal));

                cv::Vec3b newPixel;
                newPixel[0] = grad;
                newPixel[1] = grad;
                newPixel[2] = grad;
                sobel.at<cv::Vec3b>(i, j) = newPixel;
            }
        }
        outputImage(sobel, "./output/sobel.jpg");
    }

    cv::Mat aloeBlending = aloeL.clone();
    {
        for (int i = 0; i < aloeL.rows; i++) {
            for (int j = 0; j < aloeL.cols; j++) {
                const cv::Vec3b pixelL = aloeL.at<cv::Vec3b>(i, j);
                const cv::Vec3b pixelR = aloeR.at<cv::Vec3b>(i, j);
                cv::Vec3b newPixel;
                double alpha = 0.5;
                newPixel[0] = static_cast<int>(alpha * pixelL[0] + (1.0 - alpha) * pixelR[0]);
                newPixel[1] = static_cast<int>(alpha * pixelL[1] + (1.0 - alpha) * pixelR[1]);
                newPixel[2] = static_cast<int>(alpha * pixelL[2] + (1.0 - alpha) * pixelR[2]);
                aloeBlending.at<cv::Vec3b>(i, j) = newPixel;
            }
        }
        errorCode += outputImage(aloeBlending, "./output/blending.jpg");
    }

    cv::Mat aloeBlending3D = aloeL.clone();
    {
        for (int i = 0; i < aloeL.rows; i++) {
            for (int j = 0; j < aloeL.cols; j++) {
                const cv::Vec3b pixelL = aloeL.at<cv::Vec3b>(i, j);
                const cv::Vec3b pixelR = aloeR.at<cv::Vec3b>(i, j);
                cv::Vec3b newPixel;
                const int grayL = (pixelL[0] + pixelL[1] + pixelL[2]) / 3;
                const int grayR = (pixelR[0] + pixelR[1] + pixelR[2]) / 3;
                double alphaR = 1.0;
                double alphaG = 0.5;
                double alphaB = 0.0;
                newPixel[0] = static_cast<int>(alphaB * grayL + (1.0 - alphaB) * grayR);
                newPixel[1] = static_cast<int>(alphaG * grayL + (1.0 - alphaG) * grayR);
                newPixel[2] = static_cast<int>(alphaR * grayL + (1.0 - alphaR) * grayR);
                aloeBlending3D.at<cv::Vec3b>(i, j) = newPixel;
            }
        }
        errorCode += outputImage(aloeBlending3D, "./output/blending3D.jpg");
    }

    // https://www.kkaneko.jp/ai/opencv/opencvpython.html
    cv::Mat firstFrame = roboCupFrames[0];
    cv::Mat copied = firstFrame.clone();
    cv::Mat gray, dilate, mask, masked, colorMasked, line, hough, contoursIm, trapezoid;
    mask = cv::Mat::zeros(firstFrame.rows, firstFrame.cols, CV_8UC1);
    outputImage(copied, "./output/copied.jpg");
    cv::cvtColor(copied, gray, cv::COLOR_BGR2GRAY);
    cv::Point points[4];
    points[0] = cv::Point(firstFrame.cols * 3 / 10, firstFrame.rows * 3 / 40);
    points[1] = cv::Point(firstFrame.cols * 13 / 20, firstFrame.rows * 3 / 40);
    points[2] = cv::Point(firstFrame.cols * 19 / 20, firstFrame.rows * 28 / 30);
    points[3] = cv::Point(firstFrame.cols * 1 / 20, firstFrame.rows * 28 / 30);
    cv::fillConvexPoly(mask, points, 4, cv::Scalar(255, 255, 255));
    firstFrame.copyTo(colorMasked, mask);
    outputImage(colorMasked, "./output/colorMasked.jpg");
    gray.copyTo(masked, mask);
    cv::inRange(masked, cv::Scalar(100, 100, 100), cv::Scalar(250, 250, 250), line);
    outputImage(line, "./output/line.jpg");
    // 確率的ハフ変換で切れてる線をつなげる
    hough = line.clone();
    // https://labs.eecs.tottori-u.ac.jp/sd/Member/oyamada/OpenCV/html/py_tutorials/py_imgproc/py_houghlines/py_houghlines.html
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(line, lines, 1.0, 0.03, 100, 64, 32);
    for (cv::Vec4i l : lines) {
        cv::Point p1(l[0], l[1]);
        cv::Point p2(l[2], l[3]);
        cv::line(hough, p1, p2, cv::Scalar(255, 255, 255));
    }
    outputImage(hough, "./output/hough.jpg");
    std::vector<std::vector<cv::Point>> contours;
    // https://qiita.com/IronNyan/items/fad82766e22cbefd8891
    // 輪郭の導出
    cv::findContours(hough, contours,cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    contoursIm = colorMasked.clone();
    for (int i = 0; i < contours.size(); i++) {
        cv::drawContours(contoursIm, contours, i, cv::Scalar(0, 250, 0));
    }
    outputImage(contoursIm, "./output/contoursIm.jpg");
    std::optional<std::vector<cv::Point>> maxAreaPolygon;
    // 多角形で近似し，その面積がもっとも大きいものを取り出す
    for (const std::vector<cv::Point>& contour : contours) {
        std::vector<cv::Point> hull, polygon;
        cv::convexHull(contour, hull);
        double epsilon = 0.02 * cv::arcLength(hull, true);
        cv::approxPolyDP(hull, polygon, epsilon, true);
        if (polygon.size() != 4) {
            continue;
        }
        if (maxAreaPolygon.has_value()) {
            if (cv::contourArea(maxAreaPolygon.value()) < cv::contourArea(contour)) {
                maxAreaPolygon = polygon;
            }
        } else {
            maxAreaPolygon = polygon;
        }
    }

    if (maxAreaPolygon.has_value()) {
        std::vector<cv::Point> hull, polygon;
        cv::drawContours(contoursIm, maxAreaPolygon.value(), 0, cv::Scalar(0, 0, 255));
        outputImage(contoursIm, "./output/hull.jpg");
        std::vector<cv::Point2d> imagePoints;
        for (cv::Point p : maxAreaPolygon.value()) {
            cv::circle(contoursIm, p, 10, cv::Scalar(0, 255, 255), cv::FILLED);
            imagePoints.emplace_back(p.x, p.y);
        }
        outputImage(contoursIm, "./output/corner.jpg");

        std::vector<cv::Point3f> fieldPoints(4);
        // ルール上のfieldと向きが違うことに注意
        const double MAX_X = 4.5;
        const double MAX_Y = 6.0;
        // 左下から時計回りに点を配置
        for (int k = 0; k < 4; k++) {
            fieldPoints[k] = cv::Point3d(static_cast<double>((2 * (k / 2) - 1)) * MAX_X, static_cast<double>((2 * (((k + 1) % 4) / 2) - 1)) * MAX_Y, 0.0);
        }

        // https://amroamroamro.github.io/mexopencv/matlab/cv.solvePnP.html
        cv::Mat cameraMat = cv::Mat::zeros(3, 3, CV_64F);
        cv::Size imageSize = contoursIm.size();
        cv::Point2d principalPoint(0.5 * imageSize.width, 0.5 * imageSize.height);
        // FOVを80度で仮定
        constexpr double fovDegree = 80.0;
        // https://www.scratchapixel.com/lessons/3d-basic-rendering/3d-viewing-pinhole-camera/how-pinhole-camera-works-part-2.html
        double f = 0.5 * std::max(imageSize.height, imageSize.width) / std::tan(0.5 * fovDegree * std::numbers::pi / 180.0);

        cameraMat.at<double>(0, 0) = f;
        cameraMat.at<double>(1, 1) = f;
        cameraMat.at<double>(0, 2) = principalPoint.x;
        cameraMat.at<double>(1, 2) = principalPoint.y;
        cameraMat.at<double>(2, 2) = 1.0f;

        const cv::Mat distortionMat = cv::Mat::zeros(4, 1, CV_64F);

        cv::Vec3d rVec, tVec;
        // Infinitesimal Plane-Based Pose Estimationを使用
        cv::solvePnP(fieldPoints, imagePoints, cameraMat, distortionMat, rVec, tVec, false, cv::SOLVEPNP_IPPE);

        // 回転行列
        cv::Mat R;
        cv::Rodrigues(rVec, R);

        // tVecをフィールド座標系でのカメラ座標へ変換
        // Rは直行行列でdetR=1なので逆行列が転置であらわされる
        cv::Mat cameraPosMat = R.t() * (-tVec);
        cv::Vec3d cameraPos(cameraPosMat.at<double>(0), cameraPosMat.at<double>(1), cameraPosMat.at<double>(2));
        cv::Mat cameraDirMat = R * cv::Vec3d(0.0, 0.0, 1.0);
        cv::Vec3d cameraDir(cameraDirMat.at<double>(0), cameraDirMat.at<double>(1), cameraDirMat.at<double>(2));

        fmt::print(fg(fmt::color::cyan),
                   "Camera position (m): x={:.3f} y={:.3f} z={:.3f}\n",
                   cameraPos[0], cameraPos[1], cameraPos[2]);
        fmt::print(fg(fmt::color::cyan),
                   "Camera dir: x={:.3f} y={:.3f} z={:.3f}\n",
                   cameraDir[0], cameraDir[1], cameraDir[2]);
    } else {
        fmt::print(fg(fmt::color::red), "couldn't find field hull\n");
    }

    return errorCode;
}
