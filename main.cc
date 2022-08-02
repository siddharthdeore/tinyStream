#include "StreamServer.h"
#define CAMERA_ID -1
// #define CAMERA_ID "rtsp://10.240.29.68:8080/h264_ulaw.sdp"
int main(int argc, char const *argv[])
{
    StreamServer serv(8080);
    
    return 0;
}
