#ifndef ICAMERA_H
#define ICAMERA_H

#pragma once
#include <vector>
#include <memory>

/**
 * @brief Camera interface class, any derived class must implement get_frame()
 * 
 */
class ICamera
{
public:
    typedef std::shared_ptr<ICamera> Ptr;
    virtual std::vector<unsigned char> get_frame() = 0;

private:
};

#endif