#ifndef BASECAMERA_H
#define BASECAMERA_H

#pragma once
#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <vector>
#include <mutex> // std::mutex, std::lock_guard
#define Filter(fx, x) fx = fx * alfa + x * (1.0 - alfa)
class BaseCamera
{
public:
    typedef std::unique_ptr<BaseCamera> Ptr;

    BaseCamera(const std::string &device_id, const bool &face_detect = false)
    {
        _face_detect = face_detect;
        _cap.open(device_id, cv::CAP_ANY);
        _cascade.load("../assets/haarcascade_frontalface_default.xml");
    }
    BaseCamera(const int &device_id, const bool &face_detect = false)
    {
        _face_detect = face_detect;
        _cap.open(device_id, cv::CAP_ANY);
        _cascade.load("../assets/haarcascade_frontalface_default.xml");
    }
    ~BaseCamera()
    {
        _cap.release();
    }
    std::vector<uchar> get_fame()
    {
        // capture frame
        _frame_mtx.lock();
        _cap.read(_frame);
        _frame_mtx.unlock();
        if (_face_detect)
        {

            cv::cvtColor(_frame, _gray, cv::COLOR_BGRA2GRAY);
            cv::resize(_gray, _smallImg, cv::Size(), 0.2, 0.2, cv::INTER_LINEAR);
            cv::equalizeHist(_smallImg, _smallImg);
            std::vector<cv::Rect> faces;
            _cascade.detectMultiScale(_smallImg, faces, 1.1, 1, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

            for (int i = 0; i < faces.size(); i++)
            {
                Filter(fRect.x, faces[i].x * 5.0);
                Filter(fRect.y, faces[i].y * 5.0);
                Filter(fRect.width, faces[i].width * 5.0);
                Filter(fRect.height, faces[i].height * 5.0);
                cv::rectangle(_frame, fRect, cv::Scalar(255, 0, 0));
            }
        }
        cv::imencode(".jpg", _frame, jpg, params);

        return jpg;
    }

private:
    cv::VideoCapture _cap;
    cv::Mat _frame;
    cv::Mat _gray, _smallImg;
    std::vector<uchar> jpg;
    std::vector<int> params{cv::IMWRITE_JPEG_QUALITY, 50};
    std::mutex _frame_mtx;
    cv::CascadeClassifier _cascade;
    cv::Rect fRect;
    bool _face_detect;
    const double alfa = 0.87;
};
#endif