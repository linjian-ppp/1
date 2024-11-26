#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// ��������ʶ����ɫ����������
pair<string, Mat> identifyColor(const Mat& frame) {
    Mat imgYCrCb;
    cvtColor(frame, imgYCrCb, COLOR_BGR2YCrCb);

    vector<Mat> ycrcb;
    split(imgYCrCb, ycrcb);

    Mat imgCr(ycrcb[1]), imgCb(ycrcb[2]);

    // ������ɫ��Cr��Cb��Χ
    Scalar lower_red1(0, 70, 50), upper_red1(10, 255, 255);
    Scalar lower_red2(170, 70, 50), upper_red2(180, 255, 255);
    Scalar lower_green(40, 40, 40), upper_green(70, 255, 255);
    Scalar lower_yellow(20, 150, 100), upper_yellow(40, 255, 255);

    Mat maskRed1, maskRed2, maskGreen, maskYellow;
    inRange(imgCr, lower_red1, upper_red1, maskRed1);
    inRange(imgCr, lower_red2, upper_red2, maskRed2);
    inRange(imgCb, lower_green, upper_green, maskGreen);
    inRange(imgCr, lower_yellow, upper_yellow, maskYellow);

    // �ϲ���ɫ����ɫͨ��
    Mat maskRed, maskYellowFinal;
    bitwise_or(maskRed1, maskRed2, maskRed);
    bitwise_or(maskRed, maskGreen, maskYellowFinal);

    // ����ÿ����ɫ�����
    double redArea = countNonZero(maskRed);
    double greenArea = countNonZero(maskGreen);
    double yellowArea = countNonZero(maskYellow);

    // ��������ж���ɫ
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

// �������ڶ�ͼ�������ǿ����
Mat denoiseImage(const Mat& frame) {
    Mat dst;
    // �Ǿֲ���ֵȥ��
    fastNlMeansDenoisingColored(frame, dst, 10, 10, 7, 21);
    // ȷ���������ͼ����ͬһ��
    Mat bilateralFiltered;
    // ˫���˲�
    bilateralFilter(dst, bilateralFiltered, 9, 75, 75);
    return bilateralFiltered;
}

int main() {
    // ���������ͷ��ȡ��������Ҫ����������·��
    VideoCapture cap(0); // 0ͨ����Ĭ������ͷ���������ͷ������1����������
    if (!cap.isOpened()) {
        cout << "�޷�������ͷ" << endl;
        return -1;
    }

    Mat frame;
    while (true) {
        cap >> frame; // ��ȡ��ǰ֡
        if (frame.empty()) {
            cout << "�޷���ȡ֡" << endl;
            break;
        }

        // ��ǿ����
        Mat denoisedFrame = denoiseImage(frame);

        // ʶ����ɫ����ȡ����
        pair<string, Mat> result = identifyColor(denoisedFrame);
        string color = result.first;
        Mat mask = result.second;

        // ����ʶ��
        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // ���Ʒ���
        Mat contourImg = denoisedFrame.clone();
        for (const auto& contour : contours) {
            if (contourArea(contour) > 100) { // ����С����
                Point2f center;
                float radius;
                minEnclosingCircle(contour, center, radius);
                // ���Ʒ���
                rectangle(contourImg, center - Point2f(radius, radius), center + Point2f(radius, radius), Scalar(0, 255, 0), 2);
            }
        }

        // �����Ͻ�����ɫ�����ʾ����ɫ
        putText(contourImg, color, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 0), 2);

        imshow("Traffic Light Color", contourImg);
        if (waitKey(30) >= 0) break;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}

