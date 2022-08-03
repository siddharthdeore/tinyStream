#ifndef BASECAMERA_H
#define BASECAMERA_H

#pragma once

#include "ICamera.h"

#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/opencv.hpp>

#include <vector>
#include <mutex> // std::mutex, std::lock_guard

#define Filter(fx, x) fx = fx * alfa + x * (1.0 - alfa)

class BaseCamera : public ICamera
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

    virtual std::vector<uchar> get_frame()
    {
        // capture frame
        _frame_mtx.lock();
        _cap.read(_frame);
        _frame_mtx.unlock();

        // if face detection enabled
        if (_face_detect)
        {

            cv::cvtColor(_frame, _gray, cv::COLOR_BGRA2GRAY);
            cv::resize(_gray, _scaled_gray, cv::Size(), 0.2, 0.2, cv::INTER_LINEAR);
            cv::equalizeHist(_scaled_gray, _scaled_gray);
            
            std::vector<cv::Rect> faces;
            _cascade.detectMultiScale(_scaled_gray, faces, 1.5, 1, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

            for (int i = 0; i < faces.size(); i++)
            {

                Filter(fRect.x, faces[i].x * 5.0);
                Filter(fRect.y, faces[i].y * 5.0);
                Filter(fRect.width, faces[i].width * 5.0);
                Filter(fRect.height, faces[i].height * 5.0);

                cv::rectangle(_frame, fRect, cv::Scalar(186, 91, 43), 1, cv::LineTypes::LINE_AA);
            }

            if (faces.size() > 0)
            {
                _inset_roi = _frame(fRect);
                cv::resize(_inset_roi, _inset_roi, cv::Size(64, 64));
            }

            cv::Mat ROI = _frame(cv::Rect(_frame.cols - 64, _frame.rows - 64, 64, 64));
            _inset_roi.copyTo(ROI);
        }

        // encode grabed frame to jpg
        cv::imencode(".jpg", _frame, jpg, params);

        return jpg;
    }

private:
    cv::VideoCapture _cap;
    cv::Mat _frame;
    cv::Mat _gray, _scaled_gray, _inset_roi;
    
    std::vector<uchar> jpg;
    std::vector<int> params{cv::IMWRITE_JPEG_QUALITY, 90};

    cv::CascadeClassifier _cascade;
    cv::Rect fRect;
    bool _face_detect;
    const double alfa = 0.87;

    std::mutex _frame_mtx;
};
#endif