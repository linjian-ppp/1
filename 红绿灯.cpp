#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// 函数用于识别颜色并返回掩码
pair<string, Mat> identifyColor(const Mat& frame) {
    Mat imgYCrCb;
    cvtColor(frame, imgYCrCb, COLOR_BGR2YCrCb);

    vector<Mat> ycrcb;
    split(imgYCrCb, ycrcb);

    Mat imgCr(ycrcb[1]), imgCb(ycrcb[2]);

    // 定义颜色的Cr和Cb范围
    Scalar lower_red1(0, 70, 50), upper_red1(10, 255, 255);
    Scalar lower_red2(170, 70, 50), upper_red2(180, 255, 255);
    Scalar lower_green(40, 40, 40), upper_green(70, 255, 255);
    Scalar lower_yellow(20, 150, 100), upper_yellow(40, 255, 255);

    Mat maskRed1, maskRed2, maskGreen, maskYellow;
    inRange(imgCr, lower_red1, upper_red1, maskRed1);
    inRange(imgCr, lower_red2, upper_red2, maskRed2);
    inRange(imgCb, lower_green, upper_green, maskGreen);
    inRange(imgCr, lower_yellow, upper_yellow, maskYellow);

    // 合并红色和绿色通道
    Mat maskRed, maskYellowFinal;
    bitwise_or(maskRed1, maskRed2, maskRed);
    bitwise_or(maskRed, maskGreen, maskYellowFinal);

    // 计算每种颜色的面积
    double redArea = countNonZero(maskRed);
    double greenArea = countNonZero(maskGreen);
    double yellowArea = countNonZero(maskYellow);

    // 根据面积判断颜色
    string color;
    if (redArea > greenArea && redArea > yellowArea) {
        color = "Red";
    } else if (greenArea > redArea && greenArea > yellowArea) {
        color = "Green";
    } else if (yellowArea > redArea && yellowArea > greenArea) {
        color = "Yellow";
    } else {
        color = "Unknown";
    }

    return make_pair(color, maskYellowFinal);
}

// 函数用于对图像进行增强降噪
Mat denoiseImage(const Mat& frame) {
    Mat dst;
    // 非局部均值去噪
    fastNlMeansDenoisingColored(frame, dst, 10, 10, 7, 21);
    // 确保输入输出图像不是同一个
    Mat bilateralFiltered;
    // 双边滤波
    bilateralFilter(dst, bilateralFiltered, 9, 75, 75);
    return bilateralFiltered;
}

int main() {
    // 从外接摄像头读取，可能需要更改索引或路径
    VideoCapture cap(0); // 0通常是默认摄像头，外接摄像头可能是1或其他索引
    if (!cap.isOpened()) {
        cout << "无法打开摄像头" << endl;
        return -1;
    }

    Mat frame;
    while (true) {
        cap >> frame; // 读取当前帧
        if (frame.empty()) {
            cout << "无法读取帧" << endl;
            break;
        }

        // 增强降噪
        Mat denoisedFrame = denoiseImage(frame);

        // 识别颜色并获取掩码
        pair<string, Mat> result = identifyColor(denoisedFrame);
        string color = result.first;
        Mat mask = result.second;

        // 轮廓识别
        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // 绘制方框
        Mat contourImg = denoisedFrame.clone();
        for (const auto& contour : contours) {
            if (contourArea(contour) > 100) { // 过滤小轮廓
                Point2f center;
                float radius;
                minEnclosingCircle(contour, center, radius);
                // 绘制方框
                rectangle(contourImg, center - Point2f(radius, radius), center + Point2f(radius, radius), Scalar(0, 255, 0), 2);
            }
        }

        // 在左上角用蓝色字体表示出颜色
        putText(contourImg, color, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 0), 2);

        imshow("Traffic Light Color", contourImg);
        if (waitKey(30) >= 0) break;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}

